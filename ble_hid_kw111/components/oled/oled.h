#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_vendor.h"

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_BUS_PORT  0
#define I2C_HW_ADDR           0x3C
#define LCD_PIXEL_CLOCK_HZ    (400 * 1000)

// The pixel number in horizontal and vertical
#define LCD_H_RES              128
#define LCD_V_RES              16

// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

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
    bool reset_display;        // true = reset display timer
} oled_message_t;

// Basic OLED functions
esp_err_t oled_init(void);
void oled_off(void);
void oled_write_text(const char* text, bool reset_display);

// Enhanced debug functions
void oled_debug_printf(const char* format, ...);
void oled_debug_error(const char* error);
void oled_debug_status(const char* status);
void oled_show_temporary(const char* text, uint32_t duration_ms);

// Message queue management
esp_err_t oled_send_message(const oled_message_t* msg);
void oled_clear_queue(void);

#ifdef __cplusplus
}
#endif