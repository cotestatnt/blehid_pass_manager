#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_BUS_PORT  0
#define I2C_HW_ADDR           0x3C
#define LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define PIN_NUM_SDA           18
#define PIN_NUM_SCL           19
#define PIN_NUM_RST           -1

// The pixel number in horizontal and vertical
#define LCD_H_RES              128
#define LCD_V_RES              16

// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

void oled_off(void);

void oled_init(void);
void oled_write_text(const char* text, bool reset_display);

#ifdef __cplusplus
}
#endif