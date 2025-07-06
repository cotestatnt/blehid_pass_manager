/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <wchar.h>
#include <locale.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_hidd.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "hid_gap.h"

static const char *TAG = "HID_KEYBOARD";
#define CASE(a, b, c)  case a: buffer[0] = b; buffer[2] = c; break;

const char* string1 = "This is a test string for the ESP HID Keyboard.\n";
const char* string2 = "Qualche carattere speciale: €£°ç.\n";
const char* string3 = "Qualche lettera accentata: àèéìòù\n";


typedef struct{
    TaskHandle_t task_hdl;
    esp_hidd_dev_t *hid_dev;
    uint8_t protocol_mode;
    uint8_t *buffer;
} local_param_t;

static local_param_t s_ble_hid_param = {0};

// Keyboard codes
#define HID_MODIFIER_NONE           0x00
#define HID_MODIFIER_LEFT_CTRL      0x01
#define HID_MODIFIER_LEFT_SHIFT     0x02
#define HID_MODIFIER_LEFT_ALT       0x04
#define HID_MODIFIER_RIGHT_CTRL     0x10
#define HID_MODIFIER_RIGHT_SHIFT    0x20
#define HID_MODIFIER_RIGHT_ALT      0x40

#define HID_SPACE                   0x2C
#define HID_DOT                     0x37
#define HID_NEWLINE                 0x28
#define HID_FSLASH                  0x38
#define HID_BSLASH                  0x31
#define HID_COMMA                   0x36
#define HID_DOT                     0x37

const unsigned char keyboardReportMap[] = { //7 bytes input (modifiers, resrvd, keys*5), 1 byte output
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0x65,        //   Usage Maximum (0x65)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    // 65 bytes
};


static esp_hid_raw_report_map_t ble_report_maps[] = {
    {
        .data = keyboardReportMap,
        .len = sizeof(keyboardReportMap)
    },
};

static esp_hid_device_config_t ble_hid_config = {
    .vendor_id          = 0x16C0,
    .product_id         = 0x05DF,
    .version            = 0x0100,
    .device_name        = "ESP Keyboard",
    .manufacturer_name  = "Espressif",
    .serial_number      = "1234567890",
    .report_maps        = ble_report_maps,
    .report_maps_len    = 1
};


static void char_to_code(uint8_t *buffer, wint_t ch)
{
	// Check if lower or upper case
	if(ch >= 'a' && ch <= 'z')
	{
		buffer[0] = 0;
		buffer[2] = (uint8_t)(4 + (ch - 'a'));  		// convert ch to HID letter, starting at a = 4
	}
	else if(ch >= 'A' && ch <= 'Z')
	{
		// Add left shift
		buffer[0] = HID_MODIFIER_LEFT_SHIFT;
		ch = ch - ('A'-'a');                    // convert ch to lower case
		buffer[2] = (uint8_t)(4 + (ch - 'a'));  // convert ch to HID letter, starting at a = 4
	}
	else if(ch >= '0' && ch <= '9') // Check if number
	{
		buffer[0] = 0;
        buffer[2] = ch == '0' ? 39 : (uint8_t)(30 + (ch - '1'));    // convert ch to HID number, starting at 1 = 30, 0 = 39
	}
	else // not a letter nor a number
	{
		switch(ch)
		{
            CASE(0x7F, HID_MODIFIER_NONE, 0x00); // null         
            CASE('/',  HID_MODIFIER_LEFT_SHIFT, 0x24);
			CASE(8,    HID_MODIFIER_NONE, 0x2A); // backspace
			CASE('-',  HID_MODIFIER_NONE, 0x38);
            CASE('\t', HID_MODIFIER_NONE, 0x2B);
            CASE(' ',  HID_MODIFIER_NONE, HID_SPACE);
			CASE('.',  HID_MODIFIER_NONE, HID_DOT);
            CASE(',',  HID_MODIFIER_NONE, HID_COMMA);
            CASE(':',  HID_MODIFIER_LEFT_SHIFT, HID_DOT);
			CASE(';',  HID_MODIFIER_LEFT_SHIFT, HID_COMMA);
            CASE('\'', HID_MODIFIER_NONE, 0x2D);
            CASE('\"', HID_MODIFIER_LEFT_SHIFT, 0x1F);
            CASE('\n', HID_MODIFIER_NONE, HID_NEWLINE);
			CASE('\\', HID_MODIFIER_NONE, 0x35);
            CASE('|',  HID_MODIFIER_LEFT_SHIFT, 0x35);
            CASE('=',  HID_MODIFIER_LEFT_SHIFT, 0x27);
            CASE('?',  HID_MODIFIER_LEFT_SHIFT, 0x2D);
			CASE('<',  HID_MODIFIER_NONE,       0x64);
			CASE('>',  HID_MODIFIER_LEFT_SHIFT, 0x64);
			CASE('@',  HID_MODIFIER_RIGHT_ALT,  0x33);
			CASE('!',  HID_MODIFIER_LEFT_SHIFT, 0x1E);
			CASE('#',  HID_MODIFIER_RIGHT_ALT,  0x34);
			CASE('$',  HID_MODIFIER_LEFT_SHIFT, 0x21);
			CASE('%',  HID_MODIFIER_LEFT_SHIFT, 0x22);
			CASE('^',  HID_MODIFIER_LEFT_SHIFT, 0x2E);
			CASE('&',  HID_MODIFIER_LEFT_SHIFT, 0x23);
			CASE('*',  HID_MODIFIER_LEFT_SHIFT, 0x30);
			CASE('(',  HID_MODIFIER_LEFT_SHIFT, 0x25);
			CASE(')',  HID_MODIFIER_LEFT_SHIFT, 0x26);
			CASE('_',  HID_MODIFIER_LEFT_SHIFT, 0x38);
			CASE('+',  HID_MODIFIER_NONE,       0x30);
			CASE('[',  HID_MODIFIER_RIGHT_ALT,  0x2F);
			CASE(']',  HID_MODIFIER_RIGHT_ALT,  0x30);
			CASE('{',  HID_MODIFIER_RIGHT_ALT,  0x24);
			CASE('}',  HID_MODIFIER_RIGHT_ALT,  0x27);
			default:
				buffer[0] = 0;
				buffer[2] = 0;
		}
	}
}


void send_key(wint_t c)
{
    static uint8_t buffer[8] = {0};
    char_to_code(buffer, c);
    
    esp_hidd_dev_input_set(s_ble_hid_param.hid_dev, 0, 1, buffer, 8);
    /* send the keyrelease event with sufficient delay */
    vTaskDelay(50 / portTICK_PERIOD_MS);
    memset(buffer, 0, sizeof(uint8_t) * 8);
    esp_hidd_dev_input_set(s_ble_hid_param.hid_dev, 0, 1, buffer, 8);
}

void send_string(const char* str) {
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
        send_key(wc);
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
    esp_hidd_dev_input_set(s_ble_hid_param.hid_dev, 0, 1, buffer, 8);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    memset(buffer, 0, sizeof(uint8_t) * 8);
    esp_hidd_dev_input_set(s_ble_hid_param.hid_dev, 0, 1, buffer, 8);
  }
}


void ble_hid_demo_task_kbd(void *pvParameters)
{
    static const char* help_string = 
    "########################################################################\n"\
    "BT hid keyboard demo usage:\n"\
    "########################################################################\n";
    /* TODO : Add support for function keys and ctrl, alt, esc, etc. */
    printf("%s\n", help_string);
     
    wint_t wc;
    while (1) {
        wc = fgetwc(stdin);
    
        switch (wc){ 
        case '1':
            send_string(string1);
            break;
        case '2':
            send_string(string2);   
            break;
        case '3':
            send_string(string3);
            break;  
        
        default:
            if (wc != EOF) {
                // printf("Key pressed: %X %c\n", wc, wc);
                send_key(wc);
            }
            break;
        }
          
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void ble_hid_task_start_up(void)
{
    if (s_ble_hid_param.task_hdl) {
        // Task already exists
        return;
    }
    /* Nimble Specific */
    xTaskCreate(ble_hid_demo_task_kbd,  
        "ble_hid_demo_task_kbd", 
        3 * 1024, 
        NULL, 
        configMAX_PRIORITIES - 3,
        &s_ble_hid_param.task_hdl
    );
}

void ble_hid_task_shut_down(void)
{
    if (s_ble_hid_param.task_hdl) {
        vTaskDelete(s_ble_hid_param.task_hdl);
        s_ble_hid_param.task_hdl = NULL;
    }
}

static void ble_hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;
    static const char *TAG = "HID_DEV_BLE";

    switch (event) {
    case ESP_HIDD_START_EVENT: {
        ESP_LOGI(TAG, "START");
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_CONNECT_EVENT: {
        ESP_LOGI(TAG, "CONNECT");
        break;
    }
    case ESP_HIDD_PROTOCOL_MODE_EVENT: {
        ESP_LOGI(TAG, "PROTOCOL MODE[%u]: %s", param->protocol_mode.map_index, param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    }
    case ESP_HIDD_CONTROL_EVENT: {
        ESP_LOGI(TAG, "CONTROL[%u]: %sSUSPEND", param->control.map_index, param->control.control ? "EXIT_" : "");
        if (param->control.control){
            // exit suspend
            ble_hid_task_start_up();
        } else {
            // suspend
            ble_hid_task_shut_down();
        }
    break;
    }
    case ESP_HIDD_OUTPUT_EVENT: {
        ESP_LOGI(TAG, "OUTPUT[%u]: %8s ID: %2u, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        break;
    }
    case ESP_HIDD_FEATURE_EVENT: {
        ESP_LOGI(TAG, "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:", param->feature.map_index, esp_hid_usage_str(param->feature.usage), param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDD_DISCONNECT_EVENT: {
        ESP_LOGI(TAG, "DISCONNECT: %s", esp_hid_disconnect_reason_str(esp_hidd_dev_transport_get(param->disconnect.dev), param->disconnect.reason));
        ble_hid_task_shut_down();
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_STOP_EVENT: {
        ESP_LOGI(TAG, "STOP");
        break;
    }
    default:
        break;
    }
    return;
}

void ble_hid_device_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_store_config_init(void);

void init_ble_keyboard(void)
{
#if HID_DEV_MODE == HIDD_IDLE_MODE
    ESP_LOGE(TAG, "Please turn on BT HID device or BLE!");
    return;
#endif
    esp_err_t ret;
    ESP_LOGI(TAG, "setting hid gap, mode:%d", HID_DEV_MODE);
    ret = esp_hid_gap_init(HID_DEV_MODE);
    ESP_ERROR_CHECK( ret );

    ret = esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_KEYBOARD, ble_hid_config.device_name);
    ESP_ERROR_CHECK( ret );
    ESP_LOGI(TAG, "setting ble device");
    ESP_ERROR_CHECK(esp_hidd_dev_init(&ble_hid_config, ESP_HID_TRANSPORT_BLE, ble_hidd_event_callback, &s_ble_hid_param.hid_dev));

    /* XXX Need to have template for store */
    ble_store_config_init();

    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
	/* Starting nimble task after gatts is initialized*/
    ret = esp_nimble_enable(ble_hid_device_host_task);
    if (ret) {
        ESP_LOGE(TAG, "esp_nimble_enable failed: %d", ret);
    }
}
