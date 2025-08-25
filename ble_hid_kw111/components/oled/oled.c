#include "oled.h"
#include "user_list.h"
#include <stdarg.h>
#include <string.h>

#define OLED_QUEUE_SIZE 5
#define OLED_TASK_STACK_SIZE 2048
#define OLED_TASK_PRIORITY 3

static const char *TAG = "OLED";
static lv_disp_t *disp;
static esp_lcd_panel_handle_t panel_handle;

// Debug message system
static QueueHandle_t oled_msg_queue = NULL;
static TaskHandle_t oled_task_handle = NULL;
static volatile bool oled_initialized = false;

// Current display state
static oled_message_t current_message = {0};
static TickType_t message_start_time = 0;

static void oled_display_message(const oled_message_t* msg) {
    if (!oled_initialized || !disp) return;
    
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0)) {
        lv_obj_t *scr = lv_disp_get_scr_act(disp);
        lv_obj_clean(scr); // Clear the display before writing new text
        
        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
        lv_label_set_text(label, msg->text);
        
        // Set color/style based on message type
        static lv_style_t style_normal, style_error, style_debug, style_status;
        static bool styles_initialized = false;
        
        if (!styles_initialized) {
            lv_style_init(&style_normal);
            lv_style_init(&style_error);
            lv_style_init(&style_debug);
            lv_style_init(&style_status);
            styles_initialized = true;
        }
        
        switch (msg->type) {
            case OLED_MSG_TYPE_ERROR:
                // For monochrome display, we can't change color but we can style differently
                lv_obj_add_style(label, &style_error, 0);
                break;
            case OLED_MSG_TYPE_DEBUG:
                lv_obj_add_style(label, &style_debug, 0);
                break;
            case OLED_MSG_TYPE_STATUS:
                lv_obj_add_style(label, &style_status, 0);
                break;
            default:
                lv_obj_add_style(label, &style_normal, 0);
                break;
        }
        
        /* Size of the screen */
        lv_obj_set_width(label, disp->driver->hor_res);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        
        // Release the mutex
        lvgl_port_unlock();
    }

    // Update current message tracking
    current_message = *msg;
    message_start_time = xTaskGetTickCount();    
}

static void oled_task(void *pvParameters) {
    oled_message_t msg;
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        // Check for new messages (non-blocking)
        if (xQueueReceive(oled_msg_queue, &msg, 0) == pdTRUE) {
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
    if (xQueueSend(oled_msg_queue, msg, 0) == pdTRUE) {
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

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ret = lvgl_port_init(&lvgl_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL port: %s", esp_err_to_name(ret));
        return ret;
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * LCD_V_RES,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }
    };
    disp = lvgl_port_add_disp(&disp_cfg);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to add LVGL display");
        return ESP_ERR_NO_MEM;
    }

    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISP_ROT_180);

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
        
    return ESP_OK;
}


void oled_off(void) {
    if (disp) {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (lvgl_port_lock(0)) {
            lv_obj_t *scr = lv_disp_get_scr_act(disp);
            lv_obj_clean(scr); // Clear the display
            // Release the mutex
            lvgl_port_unlock();
        }
        ESP_LOGI(TAG, "Turning off OLED display");
        esp_lcd_panel_disp_on_off(panel_handle, false);
    }
    
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