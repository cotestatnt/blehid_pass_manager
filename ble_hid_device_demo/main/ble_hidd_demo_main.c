/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "driver/gpio.h"
#include "hid_dev.h"

/**
 * Brief:
 * This example Implemented BLE HID device profile related functions, in which the HID device
 * has 4 Reports (1 is mouse, 2 is keyboard and LED, 3 is Consumer Devices, 4 is Vendor devices).
 * Users can choose different reports according to their own application scenarios.
 * BLE HID profile inheritance and USB HID class.
 */

/**
 * Note:
 * 1. Win10 does not support vendor report , So SUPPORT_REPORT_VENDOR is always set to FALSE, it defines in hidd_le_prf_int.h
 * 2. Update connection parameters are not allowed during iPhone HID encryption, slave turns
 * off the ability to automatically update connection parameters during encryption.
 * 3. After our HID device is connected, the iPhones write 1 to the Report Characteristic Configuration Descriptor,
 * even if the HID encryption is not completed. This should actually be written 1 after the HID encryption is completed.
 * we modify the permissions of the Report Characteristic Configuration Descriptor to `ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENCRYPTED`.
 * if you got `GATT_INSUF_ENCRYPTION` error, please ignore.
 */

#define HID_DEMO_TAG "HID_DEMO"

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

static uint16_t hid_conn_id = 0;
static bool sec_conn = false;
static bool send_volum_up = false;

#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))
#define CASE(a, b, c)  case a: buffer[0] = b; buffer[2] = c; break;

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

#define HIDD_DEVICE_NAME            "HID"
static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x03c0,       //HID Generic,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x30,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch(event) {
        case ESP_HIDD_EVENT_REG_FINISH: {
            if (param->init_finish.state == ESP_HIDD_INIT_OK) {
                //esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
                esp_ble_gap_set_device_name(HIDD_DEVICE_NAME);
                esp_ble_gap_config_adv_data(&hidd_adv_data);

            }
            break;
        }
        case ESP_BAT_EVENT_REG: {
            break;
        }
        case ESP_HIDD_EVENT_DEINIT_FINISH:
	     break;
		case ESP_HIDD_EVENT_BLE_CONNECT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
            hid_conn_id = param->connect.conn_id;
            break;
        }
        case ESP_HIDD_EVENT_BLE_DISCONNECT: {
            sec_conn = false;
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        }
        case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT: {
            ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
            ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->vendor_write.data, param->vendor_write.length);
            break;
        }
        case ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT");
            ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->led_write.data, param->led_write.length);
            break;
        }
        default:
            break;
    }
    return;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
     case ESP_GAP_BLE_SEC_REQ_EVT:
        for(int i = 0; i < ESP_BD_ADDR_LEN; i++) {
             ESP_LOGD(HID_DEMO_TAG, "%x:",param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
	 break;
     case ESP_GAP_BLE_AUTH_CMPL_EVT:
        sec_conn = true;
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(HID_DEMO_TAG, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(HID_DEMO_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(HID_DEMO_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if(!param->ble_security.auth_cmpl.success) {
            ESP_LOGE(HID_DEMO_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        }
        break;
    default:
        break;
    }
}

static void char_to_code(uint8_t *buffer, wint_t ch)
{
	// Check if lower or upper case
	if(ch >= 'a' && ch <= 'z')
	{
		buffer[0] = 0;
		// convert ch to HID letter, starting at a = 4
		buffer[2] = (uint8_t)(4 + (ch - 'a'));
	}
	else if(ch >= 'A' && ch <= 'Z')
	{
		// Add left shift
		buffer[0] = HID_MODIFIER_LEFT_SHIFT;
		// convert ch to lower case
		ch = ch - ('A'-'a');
		// convert ch to HID letter, starting at a = 4
		buffer[2] = (uint8_t)(4 + (ch - 'a'));
	}
	else if(ch >= '0' && ch <= '9') // Check if number
	{
		buffer[0] = 0;
		// convert ch to HID number, starting at 1 = 30, 0 = 39
		buffer[2] = ch == '0' ? 39 : (uint8_t)(30 + (ch - '1'));  
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
            default:   buffer[0] = 0; buffer[2] = 0;
        }
	}
}

void send_keyboard(wint_t c)
{
    uint8_t buffer[8] = {0}; // HID report: [modifier, reserved, key1, key2, key3, key4, key5, key6]
    char_to_code(buffer, c);

    // Send key press
    esp_hidd_send_keyboard_value(hid_conn_id, buffer[0], &buffer[2], 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Send key release
    uint8_t release_key = 0;
    esp_hidd_send_keyboard_value(hid_conn_id, 0, &release_key, 0);
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
        send_keyboard(wc);
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
    esp_hidd_send_keyboard_value(hid_conn_id, buffer[0], &buffer[2], 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Send key release
    uint8_t release_key = 0;
    esp_hidd_send_keyboard_value(hid_conn_id, 0, &release_key, 0);
  }
}


void hid_demo_task(void *pvParameters)
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
        if (wc != EOF) {
            // printf("Key pressed: %X %c\n", wc, wc);
            send_keyboard(wc);
        }          
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // char c;
    // while (1) {
    //     c = fgetc(stdin);
    //     if(c != 255) {
    //         // printf("You typed: %c\n", c);
    //         send_keyboard(c);
    //     }
    //     vTaskDelay(10 / portTICK_PERIOD_MS);
    // }

    // while(1) {
    //     vTaskDelay(2000 / portTICK_PERIOD_MS);
    //     if (sec_conn) {
    //         ESP_LOGI(HID_DEMO_TAG, "Send the volume");
    //         send_volum_up = true;
    //         uint8_t key_vaule = {HID_KEY_A};
    //         esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_vaule, 1);
    //         esp_hidd_send_consumer_value(hid_conn_id, HID_CONSUMER_VOLUME_UP, true);
    //         vTaskDelay(3000 / portTICK_PERIOD_MS);
    //         if (send_volum_up) {
    //             send_volum_up = false;
    //             esp_hidd_send_consumer_value(hid_conn_id, HID_CONSUMER_VOLUME_UP, false);
    //             esp_hidd_send_consumer_value(hid_conn_id, HID_CONSUMER_VOLUME_DOWN, true);
    //             vTaskDelay(3000 / portTICK_PERIOD_MS);
    //             esp_hidd_send_consumer_value(hid_conn_id, HID_CONSUMER_VOLUME_DOWN, false);
    //         }
    //     }        
    // }
}


void app_main(void)
{
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s initialize controller failed", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s enable controller failed", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
        return;
    }

    if((ret = esp_hidd_profile_init()) != ESP_OK) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
    }

    ///register the callback function to the gap module
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    xTaskCreate(&hid_demo_task, "hid_task", 2048, NULL, 5, NULL);
}
