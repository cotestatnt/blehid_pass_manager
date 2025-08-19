/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "ble_device_main.h"

#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default
static const char *TAG = "example";

/************* TinyUSB descriptors ****************/

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    // TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};

/**
 * @brief String descriptor
 */
const char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "TinyUSB",             // 1: Manufacturer
    "TinyUSB Device",      // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "PassMan HID",         // 4: HID
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

/********* Application ***************/
void usb_send_keyboard(wint_t c)
{
    uint8_t keycode[6] = {0};
    char_to_code(keycode, c);

    // Send key press
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Send key release    
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
}

void usb_send_string(const char* str) {
  size_t i = 0;
  while (str[i]) {
    wint_t wc = (wint_t)str[i];
    uint32_t codepoint = 0x00;
    if ((wc & 0x80) != 0) {
        // Multi-byte UTF-8 character, print all bytes
        int len = 1;
        int pos = i;
        if ((wc & 0xE0) == 0xC0) len = 2;
        else if ((wc & 0xF0) == 0xE0) len = 3;
        else if ((wc & 0xF8) == 0xF0) len = 4;
        // printf("multi-byte char ");
        for (int j = 0; j < len; ++j) {
            // printf("0x%02X ", (unsigned char)str[pos + j]);
            codepoint = (codepoint << 8) | (unsigned char)str[pos + j];
            i += 1;
        }
        // printf(" - Codepoint %lX: \n", codepoint);
    } 
    else {
        // printf("single byte char: 0x%X\n", wc);
        usb_send_keyboard(wc);
        i += 1;
        continue;
    }

    // Convert codepoint to UTF-8 and handle special characters
    uint8_t buffer[8] = {0};
    switch (codepoint) {
      case 0xE282AC: buffer[0] = HID_MODIFIER_RIGHT_ALT; buffer[2] = 0x08; break;  // €
      case 0xC2A3: buffer[0] = HID_MODIFIER_LEFT_SHIFT; buffer[2] = 0x20;  break;  // £
      case 0xC2B0: buffer[0] = HID_MODIFIER_LEFT_SHIFT; buffer[2] = 0x34;  break;  // °
      case 0xC3A7: buffer[0] = HID_MODIFIER_LEFT_SHIFT; buffer[2] = 0x33;  break;  // ç
      case 0xC3A0: buffer[2] = 0x34; break;                                        // à
      case 0xC3A8: buffer[2] = 0x2F; break;                                        // è
      case 0xC3A9: buffer[0] = HID_MODIFIER_LEFT_SHIFT; buffer[2] = 0x2F; break;   // é
      case 0xC3AC: buffer[2] = 0x2E; break;                                        // ì
      case 0xC3B2: buffer[2] = 0x33; break;                                        // ò
      case 0xC3B9: buffer[2] = 0x31; break;                                        // ù
    }
    
    // Send the special key
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, buffer);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Send key release    
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
  }
}


// void usb_send_key_combination(uint8_t modifiers, uint8_t key)
// {
//     uint8_t buffer[8] = {0}; // HID report: [modifier, reserved, key1, key2, key3, key4, key5, key6]
    
//     buffer[0] = modifiers;  // Combinazione di modificatori
//     buffer[2] = key;        // Tasto principale
    
//     // Send key press
//     esp_hidd_send_keyboard_value(hid_conn_id, buffer[0], &buffer[2], 1);
//     vTaskDelay(10 / portTICK_PERIOD_MS);
    
//     // Send key release
//     uint8_t release_key = 0;
//     esp_hidd_send_keyboard_value(hid_conn_id, 0, &release_key, 0);
// }


void usb_device_init(void)
{
   
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif // TUD_OPT_HIGH_SPEED
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");    
}
