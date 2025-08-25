#pragma once
#ifndef HID_DEVICE_BLE_H
#define HID_DEVICE_BLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MTU_SIZE                128

void ble_userlist_set_authenticated(bool value);
bool ble_userlist_is_authenticated();

void ble_send_char(wint_t chr);
void ble_send_key_combination(uint8_t modifiers, uint8_t key);
void ble_send_string(const char* str);

esp_err_t ble_device_init(void);

#ifdef __cplusplus
}
#endif

#endif