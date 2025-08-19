#ifndef HID_DEVICE_USB_H    
#define HID_DEVICE_USB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void usb_send_string(const char* str);
void usb_device_init(void);

// void usb_send_key_combination(uint8_t modifiers, uint8_t key);


#ifdef __cplusplus
}
#endif

#endif