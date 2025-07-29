
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
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

// Local components
#include "ble_device_main.h"
#include "user_list.h"
#include "r503.h"
#include "oled.h"

#define PIN_NUM_FP           5
#define PIN_NUM_UP           6
#define PIN_NUM_DW           7

static const char *TAG = "MAIN";

static bool go_to_deep_sleep = false;
void enter_deep_sleep() {
    gpio_set_level((gpio_num_t)PIN_NUM_FP, 0);
    esp_deep_sleep_enable_gpio_wakeup((1ULL << TOUCH_GPIO), ESP_GPIO_WAKEUP_GPIO_LOW); 
    esp_deep_sleep_start();
}

void button_task(void *pvParameters)
{
    int last_btn_up = 1, last_btn_down = 1;

    while (1) {
        int btn_up = gpio_get_level((gpio_num_t)PIN_NUM_UP);
        int btn_down = gpio_get_level((gpio_num_t)PIN_NUM_DW);

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

        if (display_reset_pending) {
            uint32_t now = xTaskGetTickCount();
            if (now - last_interaction_time > pdMS_TO_TICKS(15000)) {  // 15 secondi
                oled_write_text("BLE PassMan", false);
                display_reset_pending = 0;
                vTaskDelay(pdMS_TO_TICKS(100));
                go_to_deep_sleep = true;                            
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

extern "C" void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Configura i pin dei pulsanti come input con pull-up
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << (gpio_num_t)PIN_NUM_UP) | (1ULL << (gpio_num_t)PIN_NUM_DW);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    // Avvia il task per la gestione dei pulsanti
    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);

    // Inizializza ed avvia il task di gestione BLE
    ble_device_init();    

    ESP_LOGI(TAG, "BLE HID Device initialized. Waiting for input...");

    // // Avvia il display OLED
    oled_init();

    // Avvia il task fingerprint
    gpio_config_t fp_conf = {};
    fp_conf.intr_type = GPIO_INTR_DISABLE;
    fp_conf.mode = GPIO_MODE_OUTPUT;
    fp_conf.pin_bit_mask = (1ULL << PIN_NUM_FP);
    fp_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    fp_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&fp_conf);
    gpio_set_level((gpio_num_t)PIN_NUM_FP, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    fingerprint_task_start();

    // Load user database
    // userdb_clear();
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    userdb_load();    
    userdb_dump();
    printFingerprintTable();
        
    while(1) {
        TickType_t now = xTaskGetTickCount();
        #if SLEEP_ENABLE        
        if (go_to_deep_sleep && now - last_interaction_time > pdMS_TO_TICKS(180000)) {
            printf("Entering deep sleep...\n");    
            go_to_deep_sleep = false;
            oled_off();
            enter_deep_sleep();
        }
        #endif
        // Aggiorna il livello della batteria ogni 30 secondi
        static TickType_t last_battery_update = 0;
        if (now - last_battery_update > pdMS_TO_TICKS(30000)) {
            // ble_battery_set_level(98);
            // ble_battery_notify_level(98);
            last_battery_update = now;
            // printf("Battery level notified: %d%%\n", 98);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}