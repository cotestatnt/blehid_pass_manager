#pragma once
#ifndef BLE_DEVICE_MAIN_H
#define BLE_DEVICE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

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

#define MAX_MTU_SIZE                128



void send_string(const char* str);
void send_key_combination(uint8_t modifiers, uint8_t key);
void ble_device_init(void);

extern uint32_t passkey;

#ifdef __cplusplus
}
#endif

#endif