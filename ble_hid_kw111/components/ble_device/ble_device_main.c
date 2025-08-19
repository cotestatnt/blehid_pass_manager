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
#include "esp_random.h"
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
#include "esp_gatt_common_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "hid_dev.h"

#include "ble_device_main.h"
#include "oled.h"

#define HID_DEMO_TAG "HID_DEMO"

static uint16_t hid_conn_id = 0;
static bool sec_conn = false;
uint32_t passkey;

#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))
#define CASE(a, b, c)  case a: buffer[0] = b; buffer[2] = c; break;

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

#define HIDD_DEVICE_NAME            "BLE PwdMan"
static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, // Slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, // Slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x03c0,   // HID Generic,
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
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


static void show_bonded_devices(void)
{
    int dev_num = esp_ble_get_bond_device_num();
    if (dev_num == 0) {
        ESP_LOGI(HID_DEMO_TAG, "Bonded devices number zero\n");
        return;
    }

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    if (!dev_list) {
        ESP_LOGI(HID_DEMO_TAG, "malloc failed, return\n");
        return;
    }
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    ESP_LOGI(HID_DEMO_TAG, "Bonded devices number %d", dev_num);
    for (int i = 0; i < dev_num; i++) {
        ESP_LOGI(HID_DEMO_TAG, "[%u] addr_type %u, addr "ESP_BD_ADDR_STR"",
                 i, dev_list[i].bd_addr_type, ESP_BD_ADDR_HEX(dev_list[i].bd_addr));        
    }
    free(dev_list);
}

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
        case ESP_BAT_EVENT_REG: 
            break;        
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
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&hidd_adv_params);        
            break;
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            //advertising start complete event to indicate advertising start successfully or failed
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(HID_DEMO_TAG, "Advertising start failed, status %x", param->adv_start_cmpl.status);
                break;
            }
            ESP_LOGI(HID_DEMO_TAG, "Advertising start successfully");
            break;
        case ESP_GAP_BLE_SEC_REQ_EVT:
            /* send the positive(true) security response to the peer device to accept the security request.
            If not accept the security request, should send the security response with negative(false) accept value*/
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
            ///show the passkey number to the user to input it in the peer device.
            ESP_LOGI(HID_DEMO_TAG, "Passkey notify, passkey %06" PRIu32, param->ble_security.key_notif.passkey);
            char passkey_str[7];
            snprintf(passkey_str, sizeof(passkey_str), "%06" PRIu32, passkey); 
            oled_write_text(passkey_str, true);
            break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT: {
            esp_bd_addr_t bd_addr;
            memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(HID_DEMO_TAG, "Authentication complete, addr_type %u, addr "ESP_BD_ADDR_STR"", param->ble_security.auth_cmpl.addr_type, ESP_BD_ADDR_HEX(bd_addr));
            if(!param->ble_security.auth_cmpl.success) {
                ESP_LOGI(HID_DEMO_TAG, "Pairing failed, reason 0x%x",param->ble_security.auth_cmpl.fail_reason);
            } else {
                ESP_LOGI(HID_DEMO_TAG, "Pairing successfully, auth_mode ESP_LE_AUTH_REQ_MITM");
            }
            show_bonded_devices();
            oled_write_text("BLE PassMan", true);            
            break;
        }
        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: {
            ESP_LOGD(HID_DEMO_TAG, "Bond device remove, status %d, device "ESP_BD_ADDR_STR"",
                    param->remove_bond_dev_cmpl.status, ESP_BD_ADDR_HEX(param->remove_bond_dev_cmpl.bd_addr));
            break;
        }
        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
            if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(HID_DEMO_TAG, "Local privacy config failed, status %x", param->local_privacy_cmpl.status);
                break;
            }
            ESP_LOGI(HID_DEMO_TAG, "Local privacy config successfully");
            break;
        default:
            break;
    }
}

void char_to_code(uint8_t *buffer, wint_t ch)
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


void send_key_combination(uint8_t modifiers, uint8_t key)
{
    uint8_t buffer[8] = {0}; // HID report: [modifier, reserved, key1, key2, key3, key4, key5, key6]
    
    buffer[0] = modifiers;  // Combinazione di modificatori
    buffer[2] = key;        // Tasto principale
    
    // Send key press
    esp_hidd_send_keyboard_value(hid_conn_id, buffer[0], &buffer[2], 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    
    // Send key release
    uint8_t release_key = 0;
    esp_hidd_send_keyboard_value(hid_conn_id, 0, &release_key, 0);
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
}

void ble_device_init(void)
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
    
    ///register the callback function to the hidd module
    if((ret = esp_hidd_register_callbacks(hidd_event_callback)) != ESP_OK) {
        ESP_LOGE(HID_DEMO_TAG, "%s register the callback function failed", __func__);
    }

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;                       //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    //set static passkey
    // uint32_t passkey = 123456;
    
    // Genera una passkey random (tra 000000 e 999999)
    passkey = esp_random() % 1000000;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    /* If your BLE device acts as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the master;
    If your BLE device acts as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    // Increare MTU size
    // The default MTU size is 23 bytes, which is too small for HID reports    
    if ((ret = esp_ble_gatt_set_local_mtu(MAX_MTU_SIZE)) != ESP_OK) {
        ESP_LOGE("BLE", "esp_ble_gatt_set_local_mtu failed: %s", esp_err_to_name(ret));
    }

    // xTaskCreate(&hid_demo_task, "hid_task", 2048, NULL, 5, NULL);
}
