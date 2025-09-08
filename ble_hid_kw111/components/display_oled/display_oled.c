/*
 * OLED display component: initializes SSD1306 over I2C, sets up LVGL, and exposes a simple text API.
 */

#include <stdio.h>
#include <string.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include <stdarg.h>
#include "esp_lcd_panel_vendor.h"

#include "display_oled.h"

static const char *TAG = "display_oled";
static const char *DEFAULT_TEXT = "BLE PassMan";

#define I2C_BUS_PORT  0
#define LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define PIN_NUM_SDA           2
#define PIN_NUM_SCL           3
#define PIN_NUM_RST           -1
#define I2C_HW_ADDR           0x3C

#if OLED_TYPE == OLED_96x16
#define LCD_H_RES 96
#define LCD_V_RES 16
#else
#define LCD_H_RES 128
#define LCD_V_RES 32
#endif

#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

#define LVGL_TICK_PERIOD_MS    5
#define LVGL_TASK_STACK_SIZE   (4 * 1024)
#define LVGL_TASK_PRIORITY     2
#define LVGL_PALETTE_SIZE      8
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1000 / CONFIG_FREERTOS_HZ

static uint8_t oled_buffer[LCD_H_RES * LCD_V_RES / 8];
static _lock_t s_lvgl_lock;
static lv_display_t *s_display = NULL;
static lv_obj_t *s_label = NULL;
static volatile bool s_lvgl_running = false;

// Handles saved for clean deinit
static i2c_master_bus_handle_t s_i2c_bus = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_timer_handle_t s_lvgl_tick_timer = NULL;
static TaskHandle_t s_lvgl_task_handle = NULL;
static void *s_lvgl_buf = NULL;

// Message manager state
static bool s_msg_active = false;
static TickType_t s_msg_expire_tick = 0;

#if OLED_TYPE == OLED_128x32
// Top bar widgets for 128x32 layout
static lv_obj_t *s_icon_ble = NULL;
static lv_obj_t *s_icon_usb = NULL;
static lv_obj_t *s_icon_bat = NULL;
static lv_obj_t *s_battery_label = NULL;
static bool s_charging = false;
static int s_charge_anim_level = 0;
static esp_timer_handle_t s_charge_timer = NULL;
#endif

#if OLED_TYPE == OLED_128x32
// Update battery bars assuming caller holds s_lvgl_lock
static void update_battery_level_unlocked(int level)
{
    if (level < 0) level = 0;
    if (level > 4) level = 4;
    if (level == 0) {
        lv_label_set_text(s_icon_bat, LV_SYMBOL_BATTERY_EMPTY);
    } else if (level == 1) {
        lv_label_set_text(s_icon_bat, LV_SYMBOL_BATTERY_1);
    } else if (level == 2) {
        lv_label_set_text(s_icon_bat, LV_SYMBOL_BATTERY_2);
    } else if (level == 3) {
        lv_label_set_text(s_icon_bat, LV_SYMBOL_BATTERY_3);
    } else {
        lv_label_set_text(s_icon_bat, LV_SYMBOL_BATTERY_FULL);
    }
}
#endif

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t io_panel, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);

    px_map += LVGL_PALETTE_SIZE;

    uint16_t hor_res = lv_display_get_physical_horizontal_resolution(disp);
    int x1 = area->x1;
    int x2 = area->x2;
    int y1 = area->y1;
    int y2 = area->y2;

    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            bool chroma_color = (px_map[(hor_res >> 3) * y  + (x >> 3)] & 1 << (7 - x % 8));
            uint8_t *buf = oled_buffer + hor_res * (y >> 3) + (x);
            if (chroma_color) {
                (*buf) &= ~(1 << (y % 8));
            } else {
                (*buf) |= (1 << (y % 8));
            }
        }
    }
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, oled_buffer);
}

static void increase_lvgl_tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (s_lvgl_running) {
        _lock_acquire(&s_lvgl_lock);
        time_till_next_ms = lv_timer_handler();
        // If a timed message expired, revert to default text
        if (s_msg_active) {
            TickType_t now = xTaskGetTickCount();
            if ((int32_t)(now - s_msg_expire_tick) >= 0) {
                s_msg_active = false;
                lv_label_set_text(s_label, DEFAULT_TEXT);
            }
        }
        _lock_release(&s_lvgl_lock);
        time_till_next_ms = MAX(time_till_next_ms, LVGL_TASK_MIN_DELAY_MS);
        time_till_next_ms = MIN(time_till_next_ms, LVGL_TASK_MAX_DELAY_MS);
        vTaskDelay(pdMS_TO_TICKS(time_till_next_ms));
    }
    // Mark task handle as finished before self-delete
    s_lvgl_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t display_oled_init(void)
{
    if (s_display) {
        return ESP_OK; // already initialized
    }

    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = PIN_NUM_SDA,
        .scl_io_num = PIN_NUM_SCL,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &i2c_bus), TAG, "i2c bus");
    s_i2c_bus = i2c_bus;

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = I2C_HW_ADDR,
        .scl_speed_hz = LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_CMD_BITS,
        .dc_bit_offset = 6,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle), TAG, "panel io");
    s_io_handle = io_handle;

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = PIN_NUM_RST,
    };
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle), TAG, "panel drv");
    s_panel_handle = panel_handle;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG, "init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), TAG, "on");
    // 180° flip 
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel_handle, true, true), TAG, "mirror");
    // Invert colors
    // ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_handle, true), TAG, "invert");

    ESP_LOGI(TAG, "Initialize LVGL");
    lv_init();

    s_display = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_user_data(s_display, panel_handle);

    size_t draw_buffer_sz = LCD_H_RES * LCD_V_RES / 8 + LVGL_PALETTE_SIZE;
    void *buf = heap_caps_calloc(1, draw_buffer_sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    assert(buf);
    s_lvgl_buf = buf;

    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_I1);
    lv_display_set_buffers(s_display, buf, NULL, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(s_display, lvgl_flush_cb);

    const esp_lcd_panel_io_callbacks_t cbs = {.on_color_trans_done = notify_lvgl_flush_ready};
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, s_display);

    const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &increase_lvgl_tick, .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_RETURN_ON_ERROR(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer), TAG, "timer create");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000), TAG, "timer start");
    s_lvgl_tick_timer = lvgl_tick_timer;

    s_lvgl_running = true;
    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, &s_lvgl_task_handle);

    // Build minimal UI: bottom centered label
    _lock_acquire(&s_lvgl_lock);
    lv_obj_t *scr = lv_display_get_screen_active(s_display);
    s_label = lv_label_create(scr);
    lv_label_set_long_mode(s_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(s_label, lv_pct(100));
    lv_obj_set_style_pad_left(s_label, 0, 0);
    lv_obj_set_style_pad_right(s_label, 0, 0);
    lv_obj_set_style_text_align(s_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_label, DEFAULT_TEXT);
    
#if OLED_TYPE == OLED_96x16
    lv_obj_align(s_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(s_label, &lv_font_montserrat_14, 0);
#else
    lv_obj_align(s_label, LV_ALIGN_BOTTOM_MID, 0, 3);
#endif

#if LV_FONT_MONTSERRAT_16 && OLED_TYPE == OLED_128x32
    lv_obj_set_style_text_font(s_label, &lv_font_montserrat_16, 0);
#endif
    _lock_release(&s_lvgl_lock);

#if OLED_TYPE == OLED_128x32
    // Build simple top bar: BLE icon, USB if active, and battery icon
    _lock_acquire(&s_lvgl_lock);
    lv_obj_t *top = lv_display_get_screen_active(s_display);

    // Left icons BLE and USB
    s_icon_ble = lv_label_create(top);
    lv_label_set_text(s_icon_ble, LV_SYMBOL_LOOP);
    lv_obj_set_pos(s_icon_ble, 0, 0);
    lv_obj_set_style_bg_opa(s_icon_ble, LV_OPA_COVER, 0);
    // hidden by default until BLE is connected
    lv_obj_add_flag(s_icon_ble, LV_OBJ_FLAG_HIDDEN);

    s_icon_usb = lv_label_create(top);
    lv_label_set_text(s_icon_usb, LV_SYMBOL_USB);
    lv_obj_set_pos(s_icon_usb, 20, 0);
    lv_obj_set_style_bg_opa(s_icon_usb, LV_OPA_COVER, 0);
    // hidden by default until USB is initialized/needed
    lv_obj_add_flag(s_icon_usb, LV_OBJ_FLAG_HIDDEN);

    // Right icon (battery container)
    s_icon_bat = lv_label_create(top);
    lv_obj_set_pos(s_icon_bat, LCD_H_RES - 20, 0);
    lv_obj_set_style_bg_opa(s_icon_bat, LV_OPA_COVER, 0);\
    // Initial battery icon shows as almost full at startup
    // lv_label_set_text(s_icon_bat, LV_SYMBOL_BATTERY_EMPTY);
    update_battery_level_unlocked(3);

//     // Battery percentage text to the left of the right icon
//     s_battery_label = lv_label_create(top);
//     lv_label_set_text(s_battery_label, "100");
// #if LV_FONT_MONTSERRAT_10
//     lv_obj_set_style_text_font(s_battery_label, &lv_font_montserrat_10, 0);
// #endif
//     // Match icon colors to the main label's text color for correct contrast
//     lv_color_t text_col = lv_obj_get_style_text_color(s_label, LV_PART_MAIN);
//     lv_obj_set_style_text_color(s_battery_label, text_col, 0);
//     lv_obj_align(s_battery_label, LV_ALIGN_TOP_RIGHT, -10, 0); 


    _lock_release(&s_lvgl_lock);
#endif

    return ESP_OK;
}

void display_oled_set_text(const char *text)
{
    if (!s_display || !s_label) return;
    _lock_acquire(&s_lvgl_lock);
    lv_label_set_text(s_label, text ? text : "");
    _lock_release(&s_lvgl_lock);
}

void display_oled_printf(const char *format, ...)
{
    if (!format) return;
    char buf[64];
    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    (void)n;
    display_oled_set_text(buf);
}

static void set_timed_message_ticks(const char *text, TickType_t duration_ticks)
{
    if (!s_display || !s_label) return;
    _lock_acquire(&s_lvgl_lock);
    lv_label_set_text(s_label, text ? text : "");
    s_msg_active = true;
    s_msg_expire_tick = xTaskGetTickCount() + duration_ticks;
    _lock_release(&s_lvgl_lock);
}

void display_oled_post_info(const char *format, ...)
{
    char buf[64];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    set_timed_message_ticks(buf, pdMS_TO_TICKS(2000));
}

void display_oled_post_error(const char *format, ...)
{
    char buf[64];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    set_timed_message_ticks(buf, pdMS_TO_TICKS(5000));
}

void display_oled_set_battery_percent(int percent)
{
#if OLED_TYPE == OLED_128x32
    if (!s_icon_bat) return;
    if (percent < 0) percent = 0;
    if (percent >= 100) percent = 100;
    _lock_acquire(&s_lvgl_lock);
    // Map percent to 4 icon states: 0,1,2,3 bars
    if (!s_charging) {
        int level = 0;
        if (percent >= 98) level = 4;
        else if (percent >= 70) level = 3;
        else if (percent >= 40) level = 2;
        else if (percent >= 15) level = 1;
        else level = 0;
        update_battery_level_unlocked(level);
    }
    _lock_release(&s_lvgl_lock);
#else
    (void)percent;
#endif
}

void display_oled_set_battery_level(int level)
{
#if OLED_TYPE == OLED_128x32
    // Only update icon bars; percent label unchanged here
    _lock_acquire(&s_lvgl_lock);
    update_battery_level_unlocked(level);
    _lock_release(&s_lvgl_lock);
#else
    (void)level;
#endif
}

void display_oled_set_ble_connected(bool connected)
{
#if OLED_TYPE == OLED_128x32
    if (!s_display || !s_icon_ble) return;
    _lock_acquire(&s_lvgl_lock);
    if (connected) lv_obj_clear_flag(s_icon_ble, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_icon_ble, LV_OBJ_FLAG_HIDDEN);
    _lock_release(&s_lvgl_lock);
#else
    (void)connected;
#endif
}

void display_oled_set_usb_initialized(bool initialized)
{
#if OLED_TYPE == OLED_128x32
    if (!s_display || !s_icon_usb) return;
    _lock_acquire(&s_lvgl_lock);
    if (initialized) lv_obj_clear_flag(s_icon_usb, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_icon_usb, LV_OBJ_FLAG_HIDDEN);
    _lock_release(&s_lvgl_lock);
#else
    (void)initialized;
#endif
}

void display_oled_deinit(void)
{
    // Stop LVGL task cleanly
    if (s_lvgl_task_handle) {
        s_lvgl_running = false;
        // wait briefly for task to exit on its own
        for (int i = 0; i < 20 && s_lvgl_task_handle != NULL; ++i) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (s_lvgl_task_handle) {
            // force delete if still running
            vTaskDelete(s_lvgl_task_handle);
            s_lvgl_task_handle = NULL;
        }
    }

    // Stop LVGL tick timer
    if (s_lvgl_tick_timer) {
        esp_timer_stop(s_lvgl_tick_timer);
        esp_timer_delete(s_lvgl_tick_timer);
        s_lvgl_tick_timer = NULL;
    }

#if OLED_TYPE == OLED_128x32
    // Stop charging timer
    if (s_charge_timer) {
        esp_timer_stop(s_charge_timer);
        esp_timer_delete(s_charge_timer);
        s_charge_timer = NULL;
    }
#endif

    // Destroy LVGL objects
    _lock_acquire(&s_lvgl_lock);
#if OLED_TYPE == OLED_128x32
    // Cleanup top bar widgets while display is still valid
    if (s_icon_ble) { lv_obj_delete(s_icon_ble); s_icon_ble = NULL; }
    if (s_icon_usb) { lv_obj_delete(s_icon_usb); s_icon_usb = NULL; }
    if (s_icon_bat)  { lv_obj_delete(s_icon_bat);  s_icon_bat  = NULL; }
    if (s_battery_label) { lv_obj_delete(s_battery_label); s_battery_label = NULL; }
#endif
    if (s_label) {
        lv_obj_delete(s_label);
        s_label = NULL;
    }
    if (s_display) {
        lv_display_delete(s_display);
        s_display = NULL;
    }
    _lock_release(&s_lvgl_lock);

    s_msg_active = false;
    s_msg_expire_tick = 0;

    if (s_lvgl_buf) {
        heap_caps_free(s_lvgl_buf);
        s_lvgl_buf = NULL;
    }

    // Turn off and delete panel
    if (s_panel_handle) {
        esp_lcd_panel_disp_on_off(s_panel_handle, false);
        esp_lcd_panel_del(s_panel_handle);
        s_panel_handle = NULL;
    }
    if (s_io_handle) {
        esp_lcd_panel_io_del(s_io_handle);
        s_io_handle = NULL;
    }
    if (s_i2c_bus) {
        i2c_del_master_bus(s_i2c_bus);
        s_i2c_bus = NULL;
    }

#if OLED_TYPE == OLED_128x32
    // Cleanup top bar widgets
    if (s_icon_ble) { lv_obj_delete(s_icon_ble); s_icon_ble = NULL; }
    if (s_icon_usb) { lv_obj_delete(s_icon_usb); s_icon_usb = NULL; }
    if (s_icon_bat)  { lv_obj_delete(s_icon_bat);  s_icon_bat  = NULL; }
    if (s_battery_label) { lv_obj_delete(s_battery_label); s_battery_label = NULL; }
#endif
}

#if OLED_TYPE == OLED_128x32
static void charge_timer_cb(void *arg)
{
    if (!s_display || !s_icon_bat) return;
    _lock_acquire(&s_lvgl_lock);
    // Cycle levels 0->1->2->3->0 ...
    update_battery_level_unlocked(s_charge_anim_level);
    s_charge_anim_level = (s_charge_anim_level + 1) % 4;
    _lock_release(&s_lvgl_lock);
}
#endif

void display_oled_set_charging(bool charging)
{
#if OLED_TYPE == OLED_128x32
    if (!s_display) return;
    if (charging == s_charging) return;
    s_charging = charging;
    if (charging) {
        if (!s_charge_timer) {
            const esp_timer_create_args_t args = {
                .callback = &charge_timer_cb,
                .name = "charge_anim"
            };
            if (esp_timer_create(&args, &s_charge_timer) != ESP_OK) return;
        }
        s_charge_anim_level = 0;
        esp_timer_start_periodic(s_charge_timer, 500000); // 500 ms
    } else {
        if (s_charge_timer) {
            esp_timer_stop(s_charge_timer);
        }
    }
#else
    (void)charging;
#endif
}
