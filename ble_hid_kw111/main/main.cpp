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


// Local components and includes
#include "config.h"
#include "battery.h"
#include "buttons.h"
#include "fingerprint.h"
#include "user_list.h"
#include "display_oled.h"
#include "hid_device_prf.h"
#include "hid_device_ble.h"

#if CONFIG_IDF_TARGET_ESP32S3
#include "hid_device_usb.h"
#endif


static const char *TAG = "MAIN";
static TaskHandle_t fingerprintTaskHandle = nullptr;
static TaskHandle_t buttonsTaskHandle = nullptr;

// Stores the last user interaction time to start deep sleep countdown
uint32_t last_interaction_time = 0;                 

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
    ret = display_oled_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize OLED: %s", esp_err_to_name(ret));
        // Continue without OLED if it fails
    }

    // Initialize and start the BLE management task
    ret = ble_device_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE initialization failed: %s", esp_err_to_name(ret));
        display_oled_post_error("BLE FAIL");
    } 
    // Reflect initial BLE connection state on OLED (hidden until connected)
    display_oled_set_ble_connected(ble_is_connected());

    // Initialize power monitoring ADC
    ret = init_power_monitoring_adc();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC initialization failed: %s", esp_err_to_name(ret));
        display_oled_post_error("ADC FAIL");
        // Continue without power monitoring if it fails
    } else {
        ESP_LOGI(TAG, "Power monitoring ADC initialized");
        // Start the battery level BLE notification task
        start_battery_notify_task();
    }

    // Start buttons task
    ESP_LOGI(TAG, "Starting button task...");
    xTaskCreate(button_task, "button_task", 4096, NULL, 5, &buttonsTaskHandle);

    // Initialize fingerprint task
    ESP_LOGI(TAG, "Starting fingerprint task...");
    xTaskCreate(fingerprint_task, "fingerprint_task", 4096, NULL, 5, &fingerprintTaskHandle);

    // Load user database
    // userdb_clear();
    vTaskDelay(pdMS_TO_TICKS(500));
    userdb_load();    
    userdb_dump();

    // Check if USB connection is available
    bool usb_available = is_usb_connected_simple();
    ESP_LOGI(TAG, "USB connection status: %s", usb_available ? "Connected" : "Not connected");
    
    #if CONFIG_IDF_TARGET_ESP32S3
    bool usb_needed = false;
    if (usb_available) {
        // Check if any user needs USB HID functionality
        for (size_t i = 0; i < user_count; ++i) {
            if (user_list[i].login_type >= 1) {
                usb_needed = true;
                break;
            }
        }

        if (usb_needed) {
            ESP_LOGI(TAG, "USB connected and needed - initializing USB HID");            
            esp_err_t ret = usb_device_init();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "USB device init failed: %s", esp_err_to_name(ret));
                display_oled_post_error("USB FAIL");                
                display_oled_set_usb_initialized(false);
            } else {
                display_oled_set_usb_initialized(true);
            }
        } else {
            ESP_LOGI(TAG, "USB connected but no users need USB HID");
            display_oled_post_info("USB Skip");
            display_oled_set_usb_initialized(false);
        }
    } else {
        ESP_LOGI(TAG, "USB not connected - skipping USB HID initialization");
        display_oled_post_info("USB NC");
        display_oled_set_usb_initialized(false);
    }
    #endif

    last_interaction_time = xTaskGetTickCount();
    while(true) {
        // Periodically re-check USB status to avoid sleeping when connected
        usb_available = is_usb_connected_simple();

        //////////////////// TEST /////////////////////
        // usb_available = false; // force sleep for testing
        //////////////////// TEST /////////////////////
        
    if (!usb_available) {
            if (xTaskGetTickCount() - last_interaction_time > pdMS_TO_TICKS(180000)) {
                #if SLEEP_ENABLE
                display_oled_deinit();
                ESP_LOGI(TAG, "Entering deep sleep...\n");
                vTaskDelay(pdMS_TO_TICKS(10));
                enter_deep_sleep();
                #endif
            }
        } else {
            // If USB is connected, keep the device awake
            last_interaction_time = xTaskGetTickCount();
        }

        // Periodically refresh icon states
        display_oled_set_ble_connected(ble_is_connected());
        #if CONFIG_IDF_TARGET_ESP32S3
        display_oled_set_usb_initialized(usb_available && usb_needed);
        #endif
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}