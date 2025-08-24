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
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


#if CONFIG_IDF_TARGET_ESP32S3
#include "driver/rtc_io.h"
#endif

// Local components
#include "config.h"
#include "fingerprint.h"
#include "ble_device_main.h"
#include "hid_device_usb.h"
#include "user_list.h"
#include "oled.h"


static const char *TAG = "MAIN";
static bool go_to_deep_sleep = false;
static TaskHandle_t fingerprintTaskHandle = nullptr;
static TaskHandle_t buttonsTaskHandle = nullptr;

// ADC per monitoraggio alimentazione (basato su esempio ESP-IDF 5.5.0)
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_vbus_handle = NULL;
static adc_cali_handle_t adc1_cali_battery_handle = NULL;  // Usato per 3.3V rail
static bool vbus_calibrated = false;
static bool battery_calibrated = false;  // Usato per 3.3V rail

#define VBUS_ADC_CHANNEL    ADC_CHANNEL_2   // GPIO 3 - VBUS con divisore integrato dev board
// #define BATTERY_ADC_CHANNEL ADC_CHANNEL_1   // GPIO 2 - non usato
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_7   // GPIO 8 - per test jumper GND/3.3V
#define ADC_ATTEN           ADC_ATTEN_DB_12
#define ADC_BITWIDTH        ADC_BITWIDTH_DEFAULT

void enter_deep_sleep() {
    gpio_set_level((gpio_num_t)FP_ACTIVATE, 0);
    
#if CONFIG_IDF_TARGET_ESP32C3
    // ESP32-C3: usa gpio wakeup
    esp_deep_sleep_enable_gpio_wakeup((1ULL << FP_TOUCH), ESP_GPIO_WAKEUP_GPIO_LOW);

#elif CONFIG_IDF_TARGET_ESP32S3
    // ESP32-S3: usa ext1 wakeup con RTC_IO
    esp_sleep_enable_ext1_wakeup((1ULL << FP_TOUCH), ESP_EXT1_WAKEUP_ANY_LOW);
    
    // Configura il pin come RTC_IO
    rtc_gpio_init((gpio_num_t)FP_TOUCH);
    rtc_gpio_set_direction((gpio_num_t)FP_TOUCH, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en((gpio_num_t)FP_TOUCH);
    rtc_gpio_pulldown_dis((gpio_num_t)FP_TOUCH);
#else
    ESP_LOGW("MAIN", "Deep sleep wakeup not configured for this target");
#endif

    esp_deep_sleep_start();
}


// Funzione di calibrazione ADC (dall'esempio ESP-IDF 5.5.0)
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

// Inizializzazione ADC per monitoraggio alimentazione (basato su esempio ESP-IDF 5.5.0)
esp_err_t init_power_monitoring_adc(void) {
    esp_err_t ret = ESP_OK;
    
    // Configurazione ADC1 oneshot
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ret = adc_oneshot_new_unit(&init_config1, &adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC1 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configurazione canale VBUS (GPIO 3) - con divisore integrato nella dev board
    adc_oneshot_chan_cfg_t vbus_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ret = adc_oneshot_config_channel(adc1_handle, VBUS_ADC_CHANNEL, &vbus_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "VBUS ADC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configurazione canale GPIO 8 - per test jumper GND/3.3V
    adc_oneshot_chan_cfg_t rail_3v3_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ret = adc_oneshot_config_channel(adc1_handle, BATTERY_ADC_CHANNEL, &rail_3v3_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO8 ADC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Calibrazione per VBUS
    vbus_calibrated = adc_calibration_init(ADC_UNIT_1, VBUS_ADC_CHANNEL, ADC_ATTEN, &adc1_cali_vbus_handle);
    
    // Calibrazione per batteria  
    battery_calibrated = adc_calibration_init(ADC_UNIT_1, BATTERY_ADC_CHANNEL, ADC_ATTEN, &adc1_cali_battery_handle);

    return ESP_OK;
}

// Lettura tensione VBUS per detection USB
int read_vbus_voltage_mv(void) {
    if (adc1_handle == NULL) {
        return -1;
    }
    
    int adc_raw = 0;
    esp_err_t ret = adc_oneshot_read(adc1_handle, VBUS_ADC_CHANNEL, &adc_raw);
    if (ret != ESP_OK) {
        return -1;
    }
    
    int voltage_mv = 0;
    if (vbus_calibrated && adc1_cali_vbus_handle) {
        ret = adc_cali_raw_to_voltage(adc1_cali_vbus_handle, adc_raw, &voltage_mv);
        if (ret != ESP_OK) {
            return -1;
        }
        
        // Fattore di compensazione empirico basato sui test:
        // Dovremmo leggere ~5000mV ma leggiamo 6292mV con compensazione 2x
        // Fattore corretto = 5000 / 6292 * 2 = 1.59
        voltage_mv = (voltage_mv * 159) / 100;  // Fattore 1.59 invece di 2.0
    } else {
        voltage_mv = (adc_raw * 3100) / 4095;
        voltage_mv = (voltage_mv * 159) / 100;  // Stesso fattore empirico
    }
    
    return voltage_mv;
}

// USB detection tramite lettura VBUS
bool is_usb_connected_simple(void) {
    int vbus_mv = read_vbus_voltage_mv();
    
    if (vbus_mv < 0) {
        ESP_LOGW(TAG, "Cannot read VBUS voltage - assuming USB disconnected");
        return false;
    }
    
    // VBUS USB dovrebbe essere ~5000mV quando connesso
    // Consideriamo connesso se > 4000mV per tolleranza
    bool usb_connected = (vbus_mv > 4000);
    ESP_LOGI(TAG, "VBUS voltage: %dmV, USB %s", vbus_mv, usb_connected ? "CONNECTED" : "DISCONNECTED");
    return usb_connected;
}

// Lettura batteria semplificata - GPIO 8
int get_battery_percentage(void) {
    if (adc1_handle == NULL) {
        return -1;
    }
    
    int adc_raw = 0;
    esp_err_t ret = adc_oneshot_read(adc1_handle, BATTERY_ADC_CHANNEL, &adc_raw);
    if (ret != ESP_OK) {
        return -1;
    }
    
    int voltage_mv = 0;
    if (battery_calibrated && adc1_cali_battery_handle) {
        ret = adc_cali_raw_to_voltage(adc1_cali_battery_handle, adc_raw, &voltage_mv);
        if (ret != ESP_OK) {
            return -1;
        }
    } else {
        voltage_mv = (adc_raw * 3100) / 4095;
    }
    
    ESP_LOGI(TAG, "Battery: %dmV, %d%%", voltage_mv, (voltage_mv * 100) / 3300);
    return (voltage_mv * 100) / 3300;  // Percentuale basata su 3.3V max
}

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
        oled_debug_error("NVS FAIL");
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

    // Initialize power monitoring ADC
    ret = init_power_monitoring_adc();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC initialization failed: %s", esp_err_to_name(ret));
        oled_debug_error("ADC FAIL");
        // Continue without power monitoring if it fails
    } else {
        ESP_LOGI(TAG, "Power monitoring ADC initialized");
    }

    // Inizializza ed avvia il task di gestione BLE
    ret = ble_device_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE initialization failed: %s", esp_err_to_name(ret));
        oled_debug_error("BLE FAIL");
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
        usb_needed = false;

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
        TickType_t now = xTaskGetTickCount();
        if (!usb_available) {
            if (go_to_deep_sleep && now - last_interaction_time > pdMS_TO_TICKS(180000)) {
                go_to_deep_sleep = false;
                oled_off();
                #if SLEEP_ENABLE
                ESP_LOGI(TAG, "Entering deep sleep...\n");
                enter_deep_sleep();
                #endif
            }
        }
        
        // Aggiorna il livello della batteria ogni 10 secondi
        static TickType_t last_battery_update = 0;
        if (now - last_battery_update > pdMS_TO_TICKS(10000)) {
            int battery_percentage = get_battery_percentage();
            if (battery_percentage >= 0) {
                // TODO: Implementare notifiche BLE batteria
                // ble_battery_set_level(battery_percentage);
                // ble_battery_notify_level(battery_percentage);
                
                // Controlla anche lo stato USB per aggiornamenti
                bool usb_status = is_usb_connected_simple();
                ESP_LOGI(TAG, "USB status: %s", usb_status ? "Connected" : "Disconnected");
            }
            last_battery_update = now;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}