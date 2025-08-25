#include "oled.h"
#include "user_list.h"
#include <stdarg.h>
#include <string.h>

#define OLED_QUEUE_SIZE 5
#define OLED_TASK_STACK_SIZE 2048
#define OLED_TASK_PRIORITY 3

static const char *TAG = "OLED";
static esp_lcd_panel_handle_t panel_handle;

// Debug message system
static QueueHandle_t oled_msg_queue = NULL;
static TaskHandle_t oled_task_handle = NULL;
static volatile bool oled_initialized = false;

// Current display state
static oled_message_t current_message = {0};
static TickType_t message_start_time = 0;

// Simple 1bpp framebuffer (128x16 => 256 bytes)
static uint8_t fb[LCD_H_RES * LCD_V_RES / 8];
// Optional tuning for panel column offset if needed
#include "sdkconfig.h"
#ifndef OLED_X_OFFSET
#ifdef CONFIG_OLED_X_OFFSET
#define OLED_X_OFFSET CONFIG_OLED_X_OFFSET
#else
#define OLED_X_OFFSET 0
#endif
#endif

#ifndef OLED_FONT_SCALE
#ifdef CONFIG_OLED_FONT_SCALE
#define OLED_FONT_SCALE CONFIG_OLED_FONT_SCALE
#else
#define OLED_FONT_SCALE 2
#endif
#endif

#ifndef OLED_FONT_HEIGHT
#ifdef CONFIG_OLED_FONT_HEIGHT
#define OLED_FONT_HEIGHT CONFIG_OLED_FONT_HEIGHT
#else
#define OLED_FONT_HEIGHT 10
#endif
#endif

// Minimal 5x7 ASCII font (columns, 5 bytes per glyph), range 0x20..0x7E
// Each byte is a column, LSB at top. One column gap added when rendering.
static const uint8_t font5x7[95][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 0x20 ' '
    {0x00,0x00,0x5F,0x00,0x00}, // 0x21 '!'
    {0x00,0x07,0x00,0x07,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '\''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x14,0x08,0x3E,0x08,0x14}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x72,0x49,0x49,0x49,0x46}, // '2'
    {0x21,0x41,0x49,0x4D,0x33}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x31}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x46,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x08,0x14,0x22,0x41,0x00}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x00,0x41,0x22,0x14,0x08}, // '>'
    {0x02,0x01,0x59,0x09,0x06}, // '?'
    {0x3E,0x41,0x5D,0x59,0x4E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\\'
    {0x00,0x41,0x41,0x7F,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x01,0x02,0x04,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x10,0x08,0x08,0x10,0x08}, // '~'
};

static inline void fb_clear(void) {
    memset(fb, 0x00, sizeof(fb));
}

static inline void fb_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= LCD_H_RES || y < 0 || y >= LCD_V_RES) return;
    size_t idx = x + (y / 8) * LCD_H_RES; // page-oriented
    uint8_t bit = 1u << (y % 8);
    if (on) fb[idx] |= bit; else fb[idx] &= (uint8_t)~bit;
}

static void fb_draw_char(int x, int y, char c) {
    if (c < 0x20 || c > 0x7E) c = '?';
    const uint8_t *glyph = font5x7[c - 0x20];
    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            bool on = bits & (1u << row);
            fb_set_pixel(x + col, y + row, on);
        }
    }
    // one pixel spacing column
}

static int text_width_px(const char *s) {
    int len = (int)strlen(s);
    if (len <= 0) return 0;
    // 5px per char + 1px space, last char no trailing space
    return len * 6 - 1;
}

// Resample 5x7 glyph to dest size using nearest-neighbor
static void fb_draw_char_resampled(int x, int y, char c, int dest_w, int dest_h) {
    if (c < 0x20 || c > 0x7E) c = '?';
    if (dest_w < 5) dest_w = 5;
    if (dest_h < 7) dest_h = 7;
    const uint8_t *glyph = font5x7[c - 0x20];
    for (int dx = 0; dx < dest_w; ++dx) {
        int sc = (dx * 5) / dest_w; // 0..4
        uint8_t colbits = glyph[sc];
        for (int dy = 0; dy < dest_h; ++dy) {
            int sr = (dy * 7) / dest_h; // 0..6
            bool on = (colbits >> sr) & 0x1;
            if (on) fb_set_pixel(x + dx, y + dy, true);
        }
    }
}

static void fb_draw_text_left_resampled(const char *text, int dest_h) {
    fb_clear();
    if (dest_h < 7) dest_h = 7;
    // Keep approximate aspect ratio: width ~ 5 * dest_h / 7
    int glyph_w = (5 * dest_h + 3) / 7;
    if (glyph_w < 5) glyph_w = 5;
    int y0 = (LCD_V_RES - dest_h) / 2; // vertical center in 16px
    if (y0 < 0) y0 = 0;
    int x = 0;
    for (const char *p = text; *p; ++p) {
        if (x + glyph_w > LCD_H_RES) break;
        fb_draw_char_resampled(x, y0, *p, glyph_w, dest_h);
        x += glyph_w + 1; // 1px spacing
    }
}

static void oled_flush(void) {
    // Draw full frame buffer to panel
    // SSD1306 expects page order; esp_lcd driver accepts a packed 1bpp buffer.
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, fb);
}

static void oled_display_message(const oled_message_t* msg) {
    if (!oled_initialized) return;
    // Fixed single size for all messages, left-aligned
    int target_h = OLED_FONT_HEIGHT;
    fb_draw_text_left_resampled(msg->text, target_h);
    oled_flush();

    // Update current message tracking
    current_message = *msg;
    message_start_time = xTaskGetTickCount();
}

static void oled_task(void *pvParameters) {
    oled_message_t msg;
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        // Check for new messages (non-blocking)
    if (xQueueReceive(oled_msg_queue, &msg, 0) == (BaseType_t)pdTRUE) {
            // New message received - display immediately (replaces current)
            oled_display_message(&msg);
        } else {
            // No new message - check if current message should be cleared
            if (current_message.display_time_ms > 0) {
                TickType_t elapsed = xTaskGetTickCount() - message_start_time;
                if (pdTICKS_TO_MS(elapsed) >= current_message.display_time_ms) {
                    // Time expired - show default message
                    oled_message_t default_msg = {
                        .text = "BLE PassMan",
                        .type = OLED_MSG_TYPE_NORMAL,
                        .display_time_ms = 0,
                        .reset_display = false
                    };
                    oled_display_message(&default_msg);
                }
            }
        }
        
        // Run at 10Hz to check for message timeouts
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
    }
}

void oled_write_text(const char* text, bool reset_display) {
    oled_message_t msg = {
        .type = OLED_MSG_TYPE_NORMAL,
        .display_time_ms = 0,  // Permanent
        .reset_display = reset_display
    };
    strncpy(msg.text, text, sizeof(msg.text) - 1);
    msg.text[sizeof(msg.text) - 1] = '\0';
    
    if (oled_initialized) {
        oled_send_message(&msg);
    }
}

// Enhanced debug functions
void oled_debug_printf(const char* format, ...) {
    if (!oled_initialized) return;
    
    oled_message_t msg = {
        .type = OLED_MSG_TYPE_DEBUG,
        .display_time_ms = 3000,  // 3 seconds
        .reset_display = false
    };
    
    va_list args;
    va_start(args, format);
    vsnprintf(msg.text, sizeof(msg.text), format, args);
    va_end(args);
    
    oled_send_message(&msg);
}

void oled_debug_error(const char* error) {
    if (!oled_initialized) return;
    
    oled_message_t msg = {
        .type = OLED_MSG_TYPE_ERROR,
        .display_time_ms = 5000,  // 5 seconds for errors
        .reset_display = false
    };
    snprintf(msg.text, sizeof(msg.text), "ERR: %s", error);
    
    oled_send_message(&msg);
}

void oled_debug_status(const char* status) {
    if (!oled_initialized) return;
    
    oled_message_t msg = {
        .type = OLED_MSG_TYPE_STATUS,
        .display_time_ms = 2000,  // 2 seconds for status
        .reset_display = false
    };
    snprintf(msg.text, sizeof(msg.text), "%s", status);
    
    oled_send_message(&msg);
}

void oled_show_temporary(const char* text, uint32_t duration_ms) {
    if (!oled_initialized) return;
    
    oled_message_t msg = {
        .type = OLED_MSG_TYPE_NORMAL,
        .display_time_ms = duration_ms,
        .reset_display = false
    };
    strncpy(msg.text, text, sizeof(msg.text) - 1);
    msg.text[sizeof(msg.text) - 1] = '\0';
    
    oled_send_message(&msg);
}

esp_err_t oled_send_message(const oled_message_t* msg) {
    if (!oled_msg_queue || !msg) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Clear queue if full to always show latest message
    if (uxQueueSpacesAvailable(oled_msg_queue) == 0) {
        oled_message_t dummy;
        xQueueReceive(oled_msg_queue, &dummy, 0);  // Remove oldest message
    }
    
    // Send new message (non-blocking)
    if (xQueueSend(oled_msg_queue, msg, 0) == (BaseType_t)pdTRUE) {
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

void oled_clear_queue(void) {
    if (oled_msg_queue) {
        xQueueReset(oled_msg_queue);
    }
}

esp_err_t oled_init(void) {
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = (gpio_num_t)PIN_NUM_SDA,
        .scl_io_num = (gpio_num_t)PIN_NUM_SCL,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = I2C_HW_ADDR,
        .scl_speed_hz = LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .lcd_cmd_bits = LCD_CMD_BITS,           // According to SSD1306 datasheet
        .lcd_param_bits = LCD_CMD_BITS,         // According to SSD1306 datasheet
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet
    };
    ret = esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = PIN_NUM_RST,
    };
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ret = esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create SSD1306 panel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_lcd_panel_reset(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset panel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init panel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to turn on panel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Rotate display 180 degrees to match previous orientation
    // Using panel mirror in both axes is equivalent to 180deg rotation
    esp_lcd_panel_mirror(panel_handle, true, true);
    // Apply horizontal gap (column offset) if configured
    if (OLED_X_OFFSET != 0) {
        esp_lcd_panel_set_gap(panel_handle, OLED_X_OFFSET, 0);
    }

    // Initialize debug message system
    ESP_LOGI(TAG, "Initialize OLED debug message system");
    oled_msg_queue = xQueueCreate(OLED_QUEUE_SIZE, sizeof(oled_message_t));
    if (oled_msg_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create OLED message queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create OLED task
    BaseType_t task_created = xTaskCreate(
        oled_task, 
        "oled_task", 
        OLED_TASK_STACK_SIZE, 
        NULL, 
        OLED_TASK_PRIORITY, 
        &oled_task_handle
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create OLED task");
        vQueueDelete(oled_msg_queue);
        oled_msg_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    oled_initialized = true;
    ESP_LOGI(TAG, "OLED debug system initialized");

    // Show default banner at startup
    oled_display_message(&(oled_message_t){ .text = "BLE PassMan", .type = OLED_MSG_TYPE_STATUS, .display_time_ms = 0 });
        
    return ESP_OK;
}


void oled_off(void) {
    // Clear the display content
    fb_clear();
    oled_flush();
    ESP_LOGI(TAG, "Turning off OLED display");
    esp_lcd_panel_disp_on_off(panel_handle, false);

    // Clean up debug system
    oled_initialized = false;
    if (oled_task_handle) {
        vTaskDelete(oled_task_handle);
        oled_task_handle = NULL;
    }
    if (oled_msg_queue) {
        vQueueDelete(oled_msg_queue);
        oled_msg_queue = NULL;
    }
}