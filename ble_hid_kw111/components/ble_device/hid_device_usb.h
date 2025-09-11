#ifndef HID_DEVICE_USB_H    
#define HID_DEVICE_USB_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void usb_send_char(wint_t chr);
void usb_send_key_combination(uint8_t modifiers, uint8_t key);
void usb_send_string(const char* str);
void usb_handle_placeholder(uint8_t ph);
esp_err_t usb_device_init(void);

#ifdef __cplusplus
}
#endif

#endif