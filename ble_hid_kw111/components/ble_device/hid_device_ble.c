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
#include "esp_mac.h"
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
#include "hid_device_ble.h"
#include "hid_device_prf.h"

#include "display_oled.h"
#include "user_list.h"

// --- PLACEHOLDER SUPPORT ----------------------------------------------------
#include "password_placeholders.h"


#define HID_DEMO_TAG        "HID BLE"
#define HIDD_DEVICE_NAME    "BLE PwdMan"

uint32_t passkey = 123456;
bool ble_userlist_authenticated = false;

uint16_t hid_conn_id = 0;
esp_bd_addr_t remote_bd_addr = {0}; // Store remote device address

// Blacklist management for preventing immediate reconnection
static esp_bd_addr_t blacklisted_addr = {0};
static uint32_t blacklist_timestamp = 0;
static const uint32_t BLACKLIST_DURATION_MS = 30000; // 30 seconds

// Whitelist management variables
static bool whitelist_enabled = false;
static int whitelist_count = 0;
#define MAX_WHITELIST_SIZE 10
static esp_bd_addr_t whitelist_devices[MAX_WHITELIST_SIZE];



static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

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

// Helper: set GAP device name to base name plus a unique suffix from MAC (e.g., BLE PwdMan-1A2B3C)
static void set_device_name_with_unique_id(void)
{
    uint8_t mac[6] = {0};
    esp_err_t err = esp_read_mac(mac, ESP_MAC_BT); // Prefer BT MAC for BLE naming
    if (err != ESP_OK) {
        // Fallback to base MAC if BT MAC not available
        err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    }

    char dev_name[32];
    // Use last 3 bytes for a short, adv-friendly suffix
    snprintf(dev_name, sizeof(dev_name), "%s-%02X%02X%02X", HIDD_DEVICE_NAME, mac[3], mac[4], mac[5]);
    esp_err_t name_err = esp_ble_gap_set_device_name(dev_name);
    if (name_err == ESP_OK) {
        ESP_LOGI(HID_DEMO_TAG, "Device name set: %s", dev_name);
    } else {
        ESP_LOGW(HID_DEMO_TAG, "Failed to set device name (%s), falling back to base name", esp_err_to_name(name_err));
        esp_ble_gap_set_device_name(HIDD_DEVICE_NAME);
    }
}

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
                // Set a unique device name to help identify the device (e.g., BLE PwdMan-1A2B3C)
                set_device_name_with_unique_id();
                esp_ble_gap_config_adv_data(&hidd_adv_data);
            }
            break;
        }
        case ESP_BAT_EVENT_REG: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_BAT_EVENT_REG");
            break;
        }
        case ESP_HIDD_EVENT_DEINIT_FINISH: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_DEINIT_FINISH");
            break;
        }
		case ESP_HIDD_EVENT_BLE_CONNECT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
            
            // Check if the connecting device is blacklisted
            if (is_device_blacklisted(param->connect.remote_bda)) {
                ESP_LOGW(HID_DEMO_TAG, "Rejecting connection from blacklisted device: "ESP_BD_ADDR_STR"", 
                         ESP_BD_ADDR_HEX(param->connect.remote_bda));
                // Immediately disconnect the blacklisted device
                esp_ble_gap_disconnect(param->connect.remote_bda);
                break;
            }
            
            hid_conn_id = param->connect.conn_id;
            // Store the remote device address for potential disconnection
            memcpy(remote_bd_addr, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            ESP_LOGI(HID_DEMO_TAG, "Connected. conn_id: %u, device: "ESP_BD_ADDR_STR"", hid_conn_id, ESP_BD_ADDR_HEX(remote_bd_addr));
            break;
        }
        case ESP_HIDD_EVENT_BLE_DISCONNECT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT - conn_id was: %u", hid_conn_id);
            ESP_LOGI(HID_DEMO_TAG, "Remote device was: "ESP_BD_ADDR_STR"", ESP_BD_ADDR_HEX(remote_bd_addr));
            
            hid_conn_id = 0; // Reset connection ID on disconnect
            memset(remote_bd_addr, 0, sizeof(esp_bd_addr_t)); // Clear remote address
            ble_userlist_authenticated = false; // Reset authentication status
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT - all connection variables reset");
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
            /* Check if device is blacklisted before accepting security request */
            if (is_device_blacklisted(param->ble_security.ble_req.bd_addr)) {
                ESP_LOGW(HID_DEMO_TAG, "Rejecting security request from blacklisted device: "ESP_BD_ADDR_STR"", 
                         ESP_BD_ADDR_HEX(param->ble_security.ble_req.bd_addr));
                esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, false);
                break;
            }
            /* send the positive(true) security response to the peer device to accept the security request.
            If not accept the security request, should send the security response with negative(false) accept value*/
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
            ///show the passkey number to the user to input it in the peer device.
            ESP_LOGI(HID_DEMO_TAG, "Passkey notify, passkey %06" PRIu32, param->ble_security.key_notif.passkey);
            char passkey_str[7];
            snprintf(passkey_str, sizeof(passkey_str), "%06" PRIu32, passkey); 
            display_oled_set_text(passkey_str);
            break;
        case ESP_GAP_BLE_AUTH_CMPL_EVT: {
            esp_bd_addr_t bd_addr;
            memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(HID_DEMO_TAG, "Authentication complete, addr_type %u, addr "ESP_BD_ADDR_STR"", param->ble_security.auth_cmpl.addr_type, ESP_BD_ADDR_HEX(bd_addr));
            if(!param->ble_security.auth_cmpl.success) {
                ESP_LOGI(HID_DEMO_TAG, "Pairing failed, reason 0x%x",param->ble_security.auth_cmpl.fail_reason);
            } else {
                ESP_LOGI(HID_DEMO_TAG, "Pairing successfully, auth_mode ESP_LE_AUTH_REQ_MITM");
                
                // Update whitelist with all bonded devices (this was a new device that just bonded)
                // But only if whitelist is currently enabled (meaning we're in filtered mode)
                if (whitelist_enabled) {
                    ESP_LOGI(HID_DEMO_TAG, "New device bonded - updating whitelist");
                    update_whitelist_from_bonded_devices();
                    update_advertising_filter_policy();
                }
            }
            show_bonded_devices();
            display_oled_set_text("BLE PassMan");            
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
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: {
            ESP_LOGI(HID_DEMO_TAG, "Connection params updated: status=%d, conn_int=%d, latency=%d, timeout=%d",
                     param->update_conn_params.status,
                     param->update_conn_params.conn_int,
                     param->update_conn_params.latency,
                     param->update_conn_params.timeout);
            break;
        }
        default:
            ESP_LOGD(HID_DEMO_TAG, "Unhandled GAP BLE event: %d", event);
            break;
    }
}


/************* Application ****************/
/******************************************/
void ble_userlist_set_authenticated(bool value) {
    ble_userlist_authenticated = value;
    send_authenticated(ble_userlist_authenticated);
}

bool ble_userlist_is_authenticated() {
    return ble_userlist_authenticated;
}

uint8_t const ble_conv_table[128][2] =  { HID_IT_IT_ASCII_TO_KEYCODE };

// Invia una sequenza HID: pressione e rilascio
static void ble_send_hid_key(uint8_t keycode[8]) {
    // HID report: [modifier, reserved, key1, key2, key3, key4, key5, key6]
    // Send key press
    esp_hidd_send_keyboard_value(hid_conn_id, keycode[0], &keycode[2], 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Send key release
    uint8_t release_key = 0;
    esp_hidd_send_keyboard_value(hid_conn_id, 0, &release_key, 0);
}

 
void ble_send_char(wint_t chr)
{    
    uint8_t keycode[8] = { 0 };
    keycode[0] = ble_conv_table[chr][0];    
    keycode[2] = ble_conv_table[chr][1];
    ble_send_hid_key(keycode);
}


void ble_send_key_combination(uint8_t modifiers, uint8_t key)
{
    uint8_t keycode[8] = {0}; 
    keycode[0] = modifiers;  // Modifier combination
    keycode[2] = key;        // Tasto principale
    ble_send_hid_key(keycode);
}

static void ble_handle_placeholder(uint8_t ph) {
    switch (ph) {
        case PW_PH_ENTER:
            ble_send_key_combination(KEYBOARD_MODIFIER_NONE, HID_KEY_ENTER); break;
        case PW_PH_TAB:
            ble_send_key_combination(KEYBOARD_MODIFIER_NONE, HID_KEY_TAB); break;
        case PW_PH_ESC:
            ble_send_key_combination(KEYBOARD_MODIFIER_NONE, HID_KEY_ESCAPE); break;
        case PW_PH_BACKSPACE:
            ble_send_key_combination(KEYBOARD_MODIFIER_NONE, HID_KEY_BACKSPACE); break;
        case PW_PH_DELAY_500MS:
            vTaskDelay(pdMS_TO_TICKS(500)); break;
        case PW_PH_DELAY_1000MS:
            vTaskDelay(pdMS_TO_TICKS(1000)); break;
        case PW_PH_CTRL_ALT_DEL:
            ble_send_key_combination(HID_MODIFIER_LEFT_CTRL | HID_MODIFIER_LEFT_ALT, HID_KEY_DELETE); break;
        case PW_PH_SHIFT_TAB:
            ble_send_key_combination(HID_MODIFIER_LEFT_SHIFT, HID_KEY_TAB); break;
        default:
            break; // Non gestito (futuro)
    }
}

void ble_send_string(const char* str) {
    size_t i = 0;
    while (str[i]) {
        uint8_t b = (uint8_t)str[i];
        if (PW_IS_PLACEHOLDER(b)) { // Gestione placeholder 0x80-0x8F
            ble_handle_placeholder(b);
            i++;
            continue;
        }

        wint_t wc = (wint_t)b;
        // ASCII semplice
        if ((wc & 0x80) == 0) {
            ble_send_char(wc);
            i++;
            continue;
        }

        // Multi-byte UTF-8 (accenti) – logica precedente
        int len = 1;
        int pos = i;
        uint32_t codepoint = 0;
        if ((wc & 0xE0) == 0xC0) len = 2;
        else if ((wc & 0xF0) == 0xE0) len = 3;
        else if ((wc & 0xF8) == 0xF0) len = 4;
        for (int j = 0; j < len; ++j) {
            codepoint = (codepoint << 8) | (unsigned char)str[pos + j];
            i++;
        }
        uint8_t keycode[8] = {0};
        switch (codepoint) {
            case 'à': keycode[0] = KEYBOARD_MODIFIER_NONE;        keycode[2] = 0x34; break;
            case 'è': keycode[0] = KEYBOARD_MODIFIER_NONE;        keycode[2] = 0x2F; break;
            case 'é': keycode[0] = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[2] = 0x2F; break;
            case 'ì': keycode[0] = KEYBOARD_MODIFIER_NONE;        keycode[2] = 0x2E; break;
            case 'ò': keycode[0] = KEYBOARD_MODIFIER_NONE;        keycode[2] = 0x33; break;
            case 'ù': keycode[0] = KEYBOARD_MODIFIER_NONE;        keycode[2] = 0x31; break;
            case '€': keycode[0] = KEYBOARD_MODIFIER_RIGHTALT;    keycode[2] = 0x08; break;
            case '£': keycode[0] = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[2] = 0x20; break;
            case 'ç': keycode[0] = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[2] = 0x33; break;
            case '§': keycode[0] = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[2] = 0x31; break;
            case '°': keycode[0] = KEYBOARD_MODIFIER_LEFTSHIFT;   keycode[2] = 0x34; break;
            default: break; // ignora se non riconosciuto
        }
        ble_send_hid_key(keycode);
    }
}

esp_err_t ble_device_init(void)
{
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s initialize controller failed", __func__);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s enable controller failed", __func__);
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed", __func__);
        return ret;
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

    // Increare MTU size. The default MTU size is 23 bytes, which is too small for HID reports    
    if ((ret = esp_ble_gatt_set_local_mtu(MAX_MTU_SIZE)) != ESP_OK) {
        ESP_LOGE("BLE", "esp_ble_gatt_set_local_mtu failed: %s", esp_err_to_name(ret));
    }
    
    // Initialize whitelist state - at startup accept all devices (bonded and non-bonded)
    whitelist_enabled = false;
    whitelist_count = 0;
    ESP_LOGI(HID_DEMO_TAG, "Whitelist initialized - accepting all connections (bonded and non-bonded)");
    
    return ret;
}

bool ble_is_connected(void)
{
    bool has_remote_addr = false;
    for (int i = 0; i < 6; i++) {
        if (remote_bd_addr[i] != 0) {
            has_remote_addr = true;
            break;
        }
    }
    return has_remote_addr;
}


esp_err_t ble_force_disconnect(void)
{
    static bool disconnect_in_progress = false;
    
    // Prevent recursive calls
    if (disconnect_in_progress) {
        ESP_LOGW(HID_DEMO_TAG, "ble_force_disconnect already in progress - ignoring call");
        return ESP_ERR_INVALID_STATE;
    }
    disconnect_in_progress = true;
    
    ESP_LOGI(HID_DEMO_TAG, "ble_force_disconnect called - hid_conn_id: %u", hid_conn_id);
    ESP_LOGI(HID_DEMO_TAG, "Remote address: "ESP_BD_ADDR_STR"", ESP_BD_ADDR_HEX(remote_bd_addr));
        
    if (ble_is_connected()) {
        ESP_LOGI(HID_DEMO_TAG, "Attempting to disconnect BLE connection");

        // Add device to blacklist before disconnecting
        blacklist_device(remote_bd_addr);
        
        // Enable whitelist filtering to prevent immediate reconnection
        enable_whitelist_filtering();
        
        esp_err_t ret = esp_ble_gap_disconnect(remote_bd_addr);
        if (ret != ESP_OK) {
            ESP_LOGE(HID_DEMO_TAG, "esp_ble_gap_disconnect failed: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(HID_DEMO_TAG, "GAP disconnect request sent successfully");
            disconnect_in_progress = false;
            return ret; // Return early on success
        }
        
        disconnect_in_progress = false;
        return ret;
    }
    
    ESP_LOGW(HID_DEMO_TAG, "No active connection to disconnect (hid_conn_id=0 and no remote address)");
    
    // Make sure advertising is running if we're not connected
    esp_err_t adv_ret = esp_ble_gap_start_advertising(&hidd_adv_params);
    if (adv_ret == ESP_OK) {
        ESP_LOGI(HID_DEMO_TAG, "Advertising restarted");
    } else {
        ESP_LOGW(HID_DEMO_TAG, "Failed to restart advertising: %s", esp_err_to_name(adv_ret));
    }
    
    disconnect_in_progress = false;
    return ESP_OK;
}

esp_err_t ble_force_disconnect_and_clear(void)
{
    ESP_LOGI(HID_DEMO_TAG, "ble_force_disconnect_and_clear called - forcing cleanup regardless of connection state");
    
    // First try the normal disconnect
    esp_err_t ret = ble_force_disconnect();
    
    // Then force cleanup regardless of the result
    ESP_LOGI(HID_DEMO_TAG, "Forcing connection state reset");
    hid_conn_id = 0;
    
    ble_userlist_authenticated = false;
    memset(remote_bd_addr, 0, sizeof(esp_bd_addr_t));
    
    // Make sure advertising is running
    ret = esp_ble_gap_start_advertising(&hidd_adv_params);
    if (ret == ESP_OK) {
        ESP_LOGI(HID_DEMO_TAG, "Advertising restarted after forced cleanup");
    } else {
        ESP_LOGW(HID_DEMO_TAG, "Failed to restart advertising after forced cleanup: %s", esp_err_to_name(ret));
    }
    
    return ESP_OK;
}

void ble_clear_blacklist(void)
{
    if (blacklist_timestamp != 0) {
        ESP_LOGI(HID_DEMO_TAG, "Manually clearing blacklist for device: "ESP_BD_ADDR_STR"", 
                 ESP_BD_ADDR_HEX(blacklisted_addr));
        memset(blacklisted_addr, 0, sizeof(esp_bd_addr_t));
        blacklist_timestamp = 0;
        
        // Return to initial state: disable whitelist to accept all devices (bonded and non-bonded)
        ESP_LOGI(HID_DEMO_TAG, "Returning to initial state - accepting all devices");
        disable_whitelist_filtering();
    } else {
        ESP_LOGI(HID_DEMO_TAG, "Blacklist is already empty");
    }
}

void ble_get_connection_info(esp_bd_addr_t *remote_addr, bool *is_bonded)
{
    if (remote_addr) {
        if (hid_conn_id != 0) {
            memcpy(remote_addr, remote_bd_addr, sizeof(esp_bd_addr_t));
        } else {
            memset(remote_addr, 0, sizeof(esp_bd_addr_t));
        }
    }
    
    if (is_bonded) {
        *is_bonded = false;
        if (hid_conn_id != 0) {
            // Get bonded devices to check if current connection is bonded
            int dev_num = esp_ble_get_bond_device_num();
            if (dev_num > 0) {
                esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
                if (dev_list) {
                    esp_ble_get_bond_device_list(&dev_num, dev_list);
                    
                    // Check if the current remote address is in the bonded devices list
                    for (int i = 0; i < dev_num; i++) {
                        if (memcmp(remote_bd_addr, dev_list[i].bd_addr, sizeof(esp_bd_addr_t)) == 0) {
                            *is_bonded = true;
                            break;
                        }
                    }
                    free(dev_list);
                }
            }
        }
    }
}

esp_err_t ble_remove_all_bonded_devices(void)
{
    esp_err_t ret = ESP_OK;
    int dev_num = esp_ble_get_bond_device_num();
    
    if (dev_num == 0) {
        ESP_LOGI(HID_DEMO_TAG, "No bonded devices to remove");
        return ESP_OK;
    }

    ESP_LOGI(HID_DEMO_TAG, "Removing %d bonded device(s)", dev_num);
    
    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    if (!dev_list) {
        ESP_LOGE(HID_DEMO_TAG, "Failed to allocate memory for device list");
        return ESP_ERR_NO_MEM;
    }
    
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    
    for (int i = 0; i < dev_num; i++) {
        ESP_LOGI(HID_DEMO_TAG, "Removing bonded device [%d]: "ESP_BD_ADDR_STR"", 
                 i, ESP_BD_ADDR_HEX(dev_list[i].bd_addr));
        
        esp_err_t remove_ret = esp_ble_remove_bond_device(dev_list[i].bd_addr);
        if (remove_ret != ESP_OK) {
            ESP_LOGE(HID_DEMO_TAG, "Failed to remove bonded device [%d]: %s", 
                     i, esp_err_to_name(remove_ret));
            ret = remove_ret;
        }
    }
    
    free(dev_list);
    
    if (ret == ESP_OK) {
        ESP_LOGI(HID_DEMO_TAG, "All bonded devices removed successfully");
    }
    
    return ret;
}


/*
Logica di funzionamento.
All'avvio: whitelist disabilitata (whitelist_enabled = false)
hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
Accetta connessioni da qualsiasi dispositivo (bonded e non bonded)

Connessione di un dispositivo non bonded: Il dispositivo può connettersi e fare pairing
Dopo il pairing completato con successo (ESP_GAP_BLE_AUTH_CMPL_EVT), se la whitelist è attiva, viene aggiornata

Pressione dei pulsanti (3 secondi): Il dispositivo corrente viene blacklistato
Si attiva la whitelist con tutti i dispositivi bonded ECCETTO quello blacklistato
hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST
Solo i dispositivi in whitelist possono connettersi

Dopo 30 secondi: La blacklist scade automaticamente
Si disattiva la whitelist e si torna alle condizioni iniziali (accetta tutti i dispositivi)
*/

// Blacklist management functions
bool is_device_blacklisted(const esp_bd_addr_t addr) {
    // Check if blacklist has expired
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (blacklist_timestamp != 0 && (current_time - blacklist_timestamp) > BLACKLIST_DURATION_MS) {
        // Blacklist expired, clear it
        memset(blacklisted_addr, 0, sizeof(esp_bd_addr_t));
        blacklist_timestamp = 0;
        ESP_LOGI(HID_DEMO_TAG, "Blacklist expired and cleared");
        return false;
    }
    
    // Check if device is blacklisted
    if (blacklist_timestamp != 0 && memcmp(blacklisted_addr, addr, sizeof(esp_bd_addr_t)) == 0) {
        uint32_t remaining_time = BLACKLIST_DURATION_MS - (current_time - blacklist_timestamp);
        ESP_LOGI(HID_DEMO_TAG, "Device "ESP_BD_ADDR_STR" is blacklisted for %lu more seconds", ESP_BD_ADDR_HEX(addr), remaining_time / 1000);
        return true;
    }
    
    return false;
}

void blacklist_device(const esp_bd_addr_t addr) {
    memcpy(blacklisted_addr, addr, sizeof(esp_bd_addr_t));
    blacklist_timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    ESP_LOGI(HID_DEMO_TAG, "Device "ESP_BD_ADDR_STR" blacklisted for %d seconds", ESP_BD_ADDR_HEX(addr), BLACKLIST_DURATION_MS / 1000);
}

// Whitelist management functions
void update_whitelist_from_bonded_devices(void) {
    // Clear current whitelist
    whitelist_count = 0;
    memset(whitelist_devices, 0, sizeof(whitelist_devices));
    
    // Get bonded devices
    int dev_num = esp_ble_get_bond_device_num();
    if (dev_num == 0) {
        ESP_LOGI(HID_DEMO_TAG, "No bonded devices for whitelist");
        return;
    }

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    if (!dev_list) {
        ESP_LOGE(HID_DEMO_TAG, "Failed to allocate memory for device list");
        return;
    }
    
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    
    // Add bonded devices to whitelist (excluding blacklisted device)
    for (int i = 0; i < dev_num && whitelist_count < MAX_WHITELIST_SIZE; i++) {
        if (!is_device_blacklisted(dev_list[i].bd_addr)) {
            memcpy(whitelist_devices[whitelist_count], dev_list[i].bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(HID_DEMO_TAG, "Added to whitelist[%d]: "ESP_BD_ADDR_STR"", whitelist_count, ESP_BD_ADDR_HEX(dev_list[i].bd_addr));
            whitelist_count++;
        } else {
            ESP_LOGI(HID_DEMO_TAG, "Excluded blacklisted device from whitelist: "ESP_BD_ADDR_STR"", ESP_BD_ADDR_HEX(dev_list[i].bd_addr));
        }
    }
    
    free(dev_list);
    ESP_LOGI(HID_DEMO_TAG, "Whitelist updated with %d devices", whitelist_count);
}

esp_err_t update_advertising_filter_policy(void) {
    extern esp_ble_adv_params_t hidd_adv_params;  // Access to global variable
    esp_err_t ret;
    
    // Stop current advertising
    ret = esp_ble_gap_stop_advertising();
    if (ret != ESP_OK) {
        ESP_LOGE(HID_DEMO_TAG, "Failed to stop advertising: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Wait a bit for advertising to stop
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    if (whitelist_enabled && whitelist_count > 0) {
        // Clear existing whitelist first
        ret = esp_ble_gap_clear_whitelist();
        if (ret != ESP_OK) {
            ESP_LOGE(HID_DEMO_TAG, "Failed to clear whitelist: %s", esp_err_to_name(ret));
        }
        
        // Add devices to whitelist
        for (int i = 0; i < whitelist_count; i++) {
            ret = esp_ble_gap_update_whitelist(true, whitelist_devices[i], BLE_ADDR_TYPE_PUBLIC);
            if (ret != ESP_OK) {
                ESP_LOGE(HID_DEMO_TAG, "Failed to add device %d to whitelist: %s", i, esp_err_to_name(ret));
            } else {
                ESP_LOGI(HID_DEMO_TAG, "Added device %d to whitelist: "ESP_BD_ADDR_STR"", i, ESP_BD_ADDR_HEX(whitelist_devices[i]));
            }
        }
        
        // Update advertising parameters to use whitelist for connections
        hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST;
        ESP_LOGI(HID_DEMO_TAG, "Advertising with whitelist filter enabled (%d devices)", whitelist_count);
    } else {
        // Clear whitelist and allow all connections
        ret = esp_ble_gap_clear_whitelist();
        if (ret != ESP_OK) {
            ESP_LOGE(HID_DEMO_TAG, "Failed to clear whitelist: %s", esp_err_to_name(ret));
        }
        
        hidd_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
        ESP_LOGI(HID_DEMO_TAG, "Advertising with no filter (allow all connections)");
    }
    
    // Restart advertising with new filter policy
    ret = esp_ble_gap_start_advertising(&hidd_adv_params);
    if (ret != ESP_OK) {
        ESP_LOGE(HID_DEMO_TAG, "Failed to restart advertising: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

void enable_whitelist_filtering(void) {
    whitelist_enabled = true;
    update_whitelist_from_bonded_devices();
    update_advertising_filter_policy();
}

void disable_whitelist_filtering(void) {
    whitelist_enabled = false;
    whitelist_count = 0;
    update_advertising_filter_policy();
}

// Public function to check and update blacklist/whitelist status
void ble_check_blacklist_expiry(void) {
    if (blacklist_timestamp != 0) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if ((current_time - blacklist_timestamp) > BLACKLIST_DURATION_MS) {
            // Blacklist expired
            memset(blacklisted_addr, 0, sizeof(esp_bd_addr_t));
            blacklist_timestamp = 0;
            ESP_LOGI(HID_DEMO_TAG, "Blacklist expired - returning to initial state (accept all devices)");
            
            // Return to initial state: disable whitelist to accept all devices (bonded and non-bonded)
            disable_whitelist_filtering();
        }
    }
}


