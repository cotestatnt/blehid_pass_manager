/* Public API for the OLED display component */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the OLED display, LVGL, and UI. Safe to call once.
esp_err_t display_oled_init(void);

// Update the text shown on the OLED. Thread-safe w.r.t. LVGL.
void display_oled_set_text(const char *text);

// printf-style helper that formats into an internal small buffer and updates the label
void display_oled_printf(const char *format, ...);

// Post a temporary message (2s) then revert to default title
void display_oled_post_info(const char *format, ...);

// Post an error message (5s) then revert to default title
void display_oled_post_error(const char *format, ...);

// Update battery percentage shown in the top bar (only visible on OLED_128x32)
void display_oled_set_battery_percent(int percent);

// Set battery icon level directly (0..3 bars) on OLED_128x32
void display_oled_set_battery_level(int level);

// Enable/disable charging animation on the battery icon (OLED_128x32)
void display_oled_set_charging(bool charging);

// Show BLE icon only when connected (OLED_128x32). No-op on other displays.
void display_oled_set_ble_connected(bool connected);

// Show USB icon only when USB HID is initialized/needed (OLED_128x32). No-op on other displays.
void display_oled_set_usb_initialized(bool initialized);

// Deinitialize and free all resources used by the OLED component
void display_oled_deinit(void);

#ifdef __cplusplus
}
#endif
