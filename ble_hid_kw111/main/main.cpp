#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <wchar.h>
#include <locale.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"


// Local components
#include "config.h"
#include "battery.h"
#include "fingerprint.h"
#include "user_list.h"
#include "oled.h"
#include "hid_device_prf.h"
#include "hid_device_ble.h"
#include "hid_device_usb.h"


static const char *TAG = "MAIN";

static TaskHandle_t fingerprintTaskHandle = nullptr;
static TaskHandle_t buttonsTaskHandle = nullptr;

void button_task(void *pvParameters)
{
    int last_btn_up = 1, last_btn_down = 1;

    // Configura i pin dei pulsanti come input con pull-up
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

        // Pulsante su (PIN 6) premuto (da HIGH a LOW)
        if (last_btn_up == 1 && btn_up == 0) {
            last_interaction_time = xTaskGetTickCount();
            user_index = (user_index + 1) ;

            if (user_index >= user_count) {
                user_index = 0;
            } 
        
            const char* username = user_list[user_index].label;                            
            printf("Account selezionato (%d): %s\n", user_index, username);
            oled_write_text(username, true);
            last_interaction_time = xTaskGetTickCount();
            display_reset_pending = 1;
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
        // Pulsante giù (PIN 7) premuto (da HIGH a LOW)
        if (last_btn_down == 1 && btn_down == 0) {
            last_interaction_time = xTaskGetTickCount();
            user_index = (user_index - 1);            
            if (user_index < 0) {
                user_index = user_count - 1;
            }

            const char* username = user_list[user_index].label;
            printf("Account selezionato (%d): %s\n", user_index, username);
            oled_write_text(username, true);
            last_interaction_time = xTaskGetTickCount();
            display_reset_pending = 1;
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

        last_btn_up = btn_up;
        last_btn_down = btn_down;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


extern "C" void app_main(void) {
    esp_err_t ret;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(ret));        
    } 

    // Start OLED display handling first
    ret = oled_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize OLED: %s", esp_err_to_name(ret));
        oled_debug_error("OLED FAIL");
        // Continue without OLED if it fails
    } else {
        // Show initial message
        oled_write_text("BLE PassMan", false);
    }    

    // Inizializza ed avvia il task di gestione BLE
    ret = ble_device_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE initialization failed: %s", esp_err_to_name(ret));
        oled_debug_error("BLE FAIL");
    }

    // Initialize power monitoring ADC
    ret = init_power_monitoring_adc();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC initialization failed: %s", esp_err_to_name(ret));
        oled_debug_error("ADC FAIL");
        // Continue without power monitoring if it fails
    } else {
        ESP_LOGI(TAG, "Power monitoring ADC initialized");
        // Avvia il task di notifica BLE del livello batteria
        start_battery_notify_task();
    }

    // Start buttons task
    ESP_LOGI(TAG, "Starting button task...");
    xTaskCreate(button_task, "button_task", 2048, NULL, 5, &buttonsTaskHandle);

    // Initialize fingerprint task
    ESP_LOGI(TAG, "Starting fingerprint task...");
    xTaskCreate(fingerprint_task, "fingerprint_task", 4096, NULL, 5, &fingerprintTaskHandle);

    // Load user database
    // userdb_clear();
    vTaskDelay(pdMS_TO_TICKS(500));
    userdb_load();    
    userdb_dump();

    // // Check if USB connection is available
    bool usb_needed = false;
    bool usb_available = is_usb_connected_simple();
    ESP_LOGI(TAG, "USB connection status: %s", usb_available ? "Connected" : "Not connected");
    
    if (usb_available) {
        // Check if any user needs USB HID functionality
        for (size_t i = 0; i < user_count; ++i) {
            if (user_list[i].login_type >= 1) {
                usb_needed = true;
                break;
            }
        }
        
        // TEST ONLY
        usb_needed = false;
        // TEST ONLY

        if (usb_needed) {
            ESP_LOGI(TAG, "USB connected and needed - initializing USB HID");            
            esp_err_t ret = usb_device_init();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "USB device init failed: %s", esp_err_to_name(ret));
                oled_debug_error("USB FAIL");
            } else {
                ESP_LOGI(TAG, "USB HID initialized successfully");
                oled_debug_status("USB Ready");
            }
        } else {
            ESP_LOGI(TAG, "USB connected but no users need USB HID");
            oled_debug_status("USB Skip");
        }
    } else {
        ESP_LOGI(TAG, "USB not connected - skipping USB HID initialization");
        oled_debug_status("USB NC");
    }

    last_interaction_time = xTaskGetTickCount();
    while(true) {
        
        // if (!usb_available) {
            if (xTaskGetTickCount() - last_interaction_time > pdMS_TO_TICKS(180000)) {                
                oled_off();
                #if SLEEP_ENABLE
                ESP_LOGI(TAG, "Entering deep sleep...\n");
                enter_deep_sleep();
                #endif
            }
        // }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}