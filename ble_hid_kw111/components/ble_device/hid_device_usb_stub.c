#include "hid_device_usb.h"
#include "esp_err.h"
#include <wchar.h>

void usb_send_char(wint_t chr) { (void)chr; }
void usb_send_key_combination(uint8_t modifiers, uint8_t key) { (void)modifiers; (void)key; }
void usb_send_string(const char* str) { (void)str; }

esp_err_t usb_device_init(void) { return ESP_OK; }
