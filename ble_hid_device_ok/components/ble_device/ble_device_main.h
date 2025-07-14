#pragma once
#ifndef BLE_DEVICE_MAIN_H
#define BLE_DEVICE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

void send_string(const char* str);
void ble_device_init(void);

extern uint32_t passkey;

#ifdef __cplusplus
}
#endif

#endif