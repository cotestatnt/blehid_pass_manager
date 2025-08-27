#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hid_device_prf.h"
#include "hid_device_ble.h"
#include "hid_device_usb.h"

#include "buttons.h"


#if CONFIG_IDF_TARGET_ESP32S3
#include "driver/rtc_io.h"
#endif


static const char *TAG = "BUTTONS";
extern uint32_t last_interaction_time;        

void button_task(void *pvParameters)
{
    int last_btn_up = 1, last_btn_down = 1;
    uint32_t both_buttons_pressed_time = 0;
    bool both_buttons_active = false;
    const uint32_t DISCONNECT_HOLD_TIME_MS = 3000; // 3 secondi

    // Configure button pins as input with pull-up
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << (gpio_num_t)BUTTON_UP) | (1ULL << (gpio_num_t)BUTTON_DOWN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    while (1) {
        int btn_up = gpio_get_level((gpio_num_t)BUTTON_UP);
        int btn_down = gpio_get_level((gpio_num_t)BUTTON_DOWN);

        // Check if both buttons are pressed simultaneously
        if ( btn_down == 0  /*&& btn_up == 0*/ ) {
            if (!both_buttons_active) {
                both_buttons_active = true;
                both_buttons_pressed_time = xTaskGetTickCount();
                ESP_LOGI(TAG, "Both buttons pressed - hold for 3 seconds to disconnect");
                oled_write_text("Hold to disconnect");
            } else {
                // Check if buttons have been held for the required time
                uint32_t current_time = xTaskGetTickCount();
                if ((current_time - both_buttons_pressed_time) >= pdMS_TO_TICKS(DISCONNECT_HOLD_TIME_MS)) {
                    bool is_connected = ble_is_connected();
                    ESP_LOGI(TAG, "Disconnect timeout reached. BLE connected: %s", is_connected ? "YES" : "NO");
                    
                    if (is_connected) {
                        ESP_LOGI(TAG, "Disconnecting BLE and restarting advertising for configuration mode");
                        oled_write_text("Disconnect BT");
                        ble_force_disconnect();
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        oled_write_text("Config Mode");
                    } else {
                        ESP_LOGI(TAG, "No BLE connection to disconnect");
                        oled_write_text("Not connected");
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        oled_write_text("BLE PassMan");
                    }
                    both_buttons_active = false;
                    last_interaction_time = xTaskGetTickCount();
                }
            }
        } else {
            if (both_buttons_active) {
                both_buttons_active = false;
                ESP_LOGI(TAG, "Both buttons released before disconnect timeout");
                oled_write_text("BLE PassMan");
            }

            // Single button logic (only if both buttons are not active)
            if (!both_buttons_active) {
                // Up button (PIN 6) pressed (HIGH to LOW transition)
                if (last_btn_up == 1 && btn_up == 0) {
                    last_interaction_time = xTaskGetTickCount();
                    user_index = (user_index + 1) ;

                    if (user_index >= user_count) {
                        user_index = 0;
                    } 
                
                    const char* username = user_list[user_index].label;                            
                    printf("Selected account (%d): %s\n", user_index, username);
                    oled_write_text(username);            
                    
                    #if DEBUG_PASSWD
                    uint8_t* encoded = user_list[user_index].password_enc;
                    size_t len = user_list[user_index].password_len;
                    char plain[128];
                    if (userdb_decrypt_password(encoded, len, plain) == 0)            
                        printf("Password (decrypted): %s\n", plain);
                    else 
                        printf("Decrypt error");
                    #endif
                    
                }
                // Down button (PIN 7) pressed (HIGH to LOW transition)
                if (last_btn_down == 1 && btn_down == 0) {
                    last_interaction_time = xTaskGetTickCount();
                    user_index = (user_index - 1);            
                    if (user_index < 0) {
                        user_index = user_count - 1;
                    }

                    const char* username = user_list[user_index].label;
                    printf("Selected account (%d): %s\n", user_index, username);
                    oled_write_text(username);            
                    
                    #if DEBUG_PASSWD
                    uint8_t* encoded = user_list[user_index].password_enc;
                    size_t len = user_list[user_index].password_len;
                    char plain[128];
                    if (userdb_decrypt_password(encoded, len, plain) == 0)
                        printf("Password (decrypted): %s\n", plain);
                    else 
                        printf("Decrypt error");
                    #endif            
                }
            }
        }

        last_btn_up = btn_up;
        last_btn_down = btn_down;
        
        // Check if blacklist has expired (chiamata periodica ogni 100ms)
        ble_check_blacklist_expiry();
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void enter_deep_sleep() {
    gpio_set_level((gpio_num_t)FP_ACTIVATE, 0);
    
#if CONFIG_IDF_TARGET_ESP32C3
    // ESP32-C3: use gpio wakeup
    esp_deep_sleep_enable_gpio_wakeup((1ULL << FP_TOUCH), ACTIVE_LEVEL ? ESP_GPIO_WAKEUP_GPIO_HIGH : ESP_GPIO_WAKEUP_GPIO_LOW);

#elif CONFIG_IDF_TARGET_ESP32S3
    // ESP32-S3: use ext1 wakeup with RTC_IO
    esp_sleep_enable_ext1_wakeup((1ULL << FP_TOUCH), ACTIVE_LEVEL ? ESP_EXT1_WAKEUP_ANY_HIGH: ESP_EXT1_WAKEUP_ANY_LOW);

    // Configure pin as RTC_IO
    rtc_gpio_init((gpio_num_t)FP_TOUCH);
    rtc_gpio_set_direction((gpio_num_t)FP_TOUCH, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en((gpio_num_t)FP_TOUCH);
    rtc_gpio_pulldown_dis((gpio_num_t)FP_TOUCH);
#else
    ESP_LOGW("MAIN", "Deep sleep wakeup not configured for this target");
#endif

    esp_deep_sleep_start();
}