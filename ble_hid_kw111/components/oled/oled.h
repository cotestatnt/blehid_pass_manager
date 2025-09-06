#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_vendor.h"

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_BUS_PORT  0
#define I2C_HW_ADDR           0x3C
#define LCD_PIXEL_CLOCK_HZ    (400 * 1000)

// The pixel number in horizontal and vertical are now defined in config.h
// based on the selected OLED_TYPE

// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

// Icon definitions (14x16 for BLE, 18x14 for USB, 20x10 for battery HORIZONTAL)
typedef enum {
    ICON_BLE_CONNECTED,
    ICON_BLE_DISCONNECTED,
    ICON_USB_CONNECTED,
    ICON_USB_DISCONNECTED,
    ICON_BATTERY_FULL,
    ICON_BATTERY_HIGH,
    ICON_BATTERY_MEDIUM,
    ICON_BATTERY_LOW,
    ICON_BATTERY_EMPTY
} oled_icon_t;

// Debug message types
typedef enum {
    OLED_MSG_TYPE_NORMAL,
    OLED_MSG_TYPE_DEBUG,
    OLED_MSG_TYPE_ERROR,
    OLED_MSG_TYPE_STATUS
} oled_msg_type_t;

typedef struct {
    char text[64];
    oled_msg_type_t type;
    uint32_t display_time_ms;  // 0 = permanent, >0 = auto-clear after this time
} oled_message_t;

// Basic OLED functions
esp_err_t oled_init(void);
void oled_off(void);
void oled_write_text(const char* text);
void oled_write_text_permanent(const char* text);

// Enhanced debug functions
void oled_debug_printf(const char* format, ...);
void oled_debug_error(const char* error);
void oled_debug_status(const char* status);

// Icon and status functions
void oled_draw_icon(int x, int y, oled_icon_t icon);
void oled_show_status_bar(bool ble_connected, bool usb_connected, int battery_percent);
void oled_update_status(bool ble_connected, bool usb_connected, int battery_percent, const char* main_text);
// void oled_show_temporary(const char* text, uint32_t duration_ms);

// Message queue management
esp_err_t oled_send_message(const oled_message_t* msg);
void oled_clear_queue(void);

#ifdef __cplusplus
}
#endif