/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_hidd_prf_api.h"
#include "hid_device_le_prf.h"
#include "hid_dev.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

// HID keyboard input report length
#define HID_KEYBOARD_IN_RPT_LEN     8

// HID LED output report length
#define HID_LED_OUT_RPT_LEN         1


esp_err_t esp_hidd_register_callbacks(esp_hidd_event_cb_t callbacks)
{
    esp_err_t hidd_status;

    if(callbacks != NULL) {
   	    hidd_le_env.hidd_cb = callbacks;
    } else {
        return ESP_FAIL;
    }

    if((hidd_status = hidd_register_cb()) != ESP_OK) {
        ESP_LOGE(HID_LE_PRF_TAG, "Failed to register HID device profile callback, status: %d", hidd_status);
        return hidd_status;
    }

    esp_ble_gatts_app_register(BATTRAY_APP_ID);

    if((hidd_status = esp_ble_gatts_app_register(HIDD_APP_ID)) != ESP_OK) {
        ESP_LOGE(HID_LE_PRF_TAG, "Failed to register HID device profile app, status: %d", hidd_status);
        return hidd_status;
    }

    return hidd_status;
}

esp_err_t esp_hidd_profile_init(void)
{
     if (hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
        return ESP_FAIL;
    }
    // Reset the hid device target environment
    memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
    hidd_le_env.enabled = true;
    return ESP_OK;
}

esp_err_t esp_hidd_profile_deinit(void)
{
    uint16_t hidd_svc_hdl = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC];
    if (!hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
        return ESP_OK;
    }

    if(hidd_svc_hdl != 0) {
	esp_ble_gatts_stop_service(hidd_svc_hdl);
	esp_ble_gatts_delete_service(hidd_svc_hdl);
    } else {
	return ESP_FAIL;
   }

    /* register the HID device profile to the BTA_GATTS module*/
    esp_ble_gatts_app_unregister(hidd_le_env.gatt_if);

    return ESP_OK;
}

uint16_t esp_hidd_get_version(void)
{
	return HIDD_VERSION;
}


void esp_hidd_send_keyboard_value(uint16_t conn_id, key_mask_t special_key_mask, uint8_t *keyboard_cmd, uint8_t num_key)
{
    if (num_key > HID_KEYBOARD_IN_RPT_LEN - 2) {
        ESP_LOGE(HID_LE_PRF_TAG, "%s(), the number key should not be more than %d", __func__, HID_KEYBOARD_IN_RPT_LEN);
        return;
    }

    uint8_t buffer[HID_KEYBOARD_IN_RPT_LEN] = {0};
    buffer[0] = special_key_mask;

    for (int i = 0; i < num_key; i++) {
        buffer[i+2] = keyboard_cmd[i];
    }

    ESP_LOGD(HID_LE_PRF_TAG, "the key vaule = %d,%d,%d, %d, %d, %d,%d, %d", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
    hid_dev_send_report(hidd_le_env.gatt_if, conn_id, HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, buffer);
    return;
}

