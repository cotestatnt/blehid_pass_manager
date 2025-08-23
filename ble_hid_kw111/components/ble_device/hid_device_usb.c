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
#include "hid_keycodes.h"


static const char *TAG = "USB HID";

/************* TinyUSB descriptors ****************/

// Enumeration for interface numbers
enum {
    ITF_NUM_HID = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_CDC_CTRL,
    ITF_NUM_TOTAL
};

// Enumeration for endpoint addresses
enum {
    EPNUM_HID = 0x81,
    EPNUM_CDC_NOTIF = 0x82,
    EPNUM_CDC_OUT = 0x03,
    EPNUM_CDC_IN = 0x83,
};

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};

/**
 * @brief Composite Configuration Descriptor
 * 
 * This descriptor defines a composite device with HID + CDC interfaces
 */
static const uint8_t composite_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface 0: HID
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, false, sizeof(hid_report_descriptor), EPNUM_HID, 16, 10),

    // Interface 1-2: CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_CTRL, 5, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

/**
 * @brief String descriptor
 */
const char* hid_string_descriptor[6] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "TinyUSB",             // 1: Manufacturer
    "TinyUSB Composite Device",      // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "Example HID interface",  // 4: HID
    "Example CDC interface",  // 5: CDC
};

/********* TinyUSB CDC callbacks ***************/

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf;
    (void) rts;

    // TODO set some indicator
    if (dtr) {
        // Terminal connected
        ESP_LOGI(TAG, "CDC connected");
    } else {
        // Terminal disconnected
        ESP_LOGI(TAG, "CDC disconnected");
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
    (void) itf;
}

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
uint8_t const usb_conv_table[128][2] =  { HID_IT_IT_ASCII_TO_KEYCODE };

// Invia una sequenza HID: pressione e rilascio
void usb_send_hid_key(uint8_t modifier, uint8_t keycode[6]) {
    // Send key press
    while (!tud_hid_ready()) vTaskDelay(pdMS_TO_TICKS(1));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keycode);
    vTaskDelay(pdMS_TO_TICKS(20));
    // Send key release
    uint8_t empty[6] = {0};
    while (!tud_hid_ready()) vTaskDelay(pdMS_TO_TICKS(1));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, empty);
    vTaskDelay(pdMS_TO_TICKS(5));
}
 
void usb_send_char(wint_t chr)
{    
    // HID report: [modifier, key1, key2, key3, key4, key5, key6]
    uint8_t keycode[6] = { 0 }; 
    uint8_t modifier   = 0;
    modifier = usb_conv_table[chr][0];    
    keycode[0] = usb_conv_table[chr][1];
    usb_send_hid_key(modifier, keycode);
}


void usb_send_string(const char* str) {
  size_t i = 0;
  while (str[i]) {    
    wint_t wc = (wint_t)str[i];

    // Single-byte character      
    if ((wc & 0x80) == 0) {          
        usb_send_char(wc);            
        i += 1;
        // Skip to next character
        continue; 
    }

    // Multi-byte UTF-8 character
    int len = 1;
    int pos = i;
    uint32_t codepoint = 0;

    if ((wc & 0xE0) == 0xC0) len = 2;
    else if ((wc & 0xF0) == 0xE0) len = 3;
    else if ((wc & 0xF8) == 0xF0) len = 4;

    for (int j = 0; j < len; ++j) {  
        codepoint = (codepoint << 8) | (unsigned char)str[pos + j];
        i += 1;
    }    
    // printf("codepoint: %lX: \n",  codepoint);

    // Convert codepoint to UTF-8 and handle special characters
    uint8_t keycode[6] = {0};
    uint8_t modifier   = 0;

    switch (codepoint) {    
        case 'à': modifier = KEYBOARD_MODIFIER_NONE;        keycode[1] = 0x34; break;
        case 'è': modifier = KEYBOARD_MODIFIER_NONE;        keycode[1] = 0x2F; break;
        case 'é': modifier = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[1] = 0x2F; break;
        case 'ì': modifier = KEYBOARD_MODIFIER_NONE;        keycode[1] = 0x2E; break;
        case 'ò': modifier = KEYBOARD_MODIFIER_NONE;        keycode[1] = 0x33; break;
        case 'ù': modifier = KEYBOARD_MODIFIER_NONE;        keycode[1] = 0x31; break;

        case '€': modifier = KEYBOARD_MODIFIER_RIGHTALT;    keycode[1] = 0x08; break; 
        case '£': modifier = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[1] = 0x20; break;
        case 'ç': modifier = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[1] = 0x33; break; 
        case '§': modifier = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[1] = 0x31; break; 
        case '°': modifier = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[1] = 0x34; break;         
    }    
    usb_send_hid_key(modifier, keycode);
  }
}

void usb_send_key_combination(uint8_t modifiers, uint8_t key)
{
    uint8_t keycode[6] = {0};    
    keycode[1] = key;       
    usb_send_hid_key(modifiers, keycode);
}


esp_err_t usb_device_init(void)
{
   
    ESP_LOGI(TAG, "USB initialization");
    
    // Ritarda l'inizializzazione per permettere al sistema di stabilizzarsi
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = composite_configuration_descriptor, 
        .hs_configuration_descriptor = composite_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = composite_configuration_descriptor,
#endif // TUD_OPT_HIGH_SPEED
    };

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "USB initialization DONE");
    return ESP_OK;
    
}
