#include "esp_log.h"
#include <stdio.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"

extern "C" {
#include "hid_device_prf.h"
}

#include "driver/usb_serial_jtag.h"

#include "display_oled.h"
#include "battery.h"

static const char *TAG = "BAT";
static TaskHandle_t batteryNotifyTaskHandle = NULL;

// ADC for power monitoring (based on ESP-IDF 5.5.0 example)
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_battery_handle = NULL;  // Used for battery voltage
static bool battery_calibrated = false;

#define ADC_ATTEN           ADC_ATTEN_DB_12
#define ADC_BITWIDTH        ADC_BITWIDTH_DEFAULT

// Exponential Moving Average (EMA) smoothing factor for battery voltage
// Higher alpha -> faster reaction, lower alpha -> smoother
#ifndef BATTERY_EMA_ALPHA
#define BATTERY_EMA_ALPHA 0.3f
#endif

// Persistent EMA state for battery voltage (in mV)
static bool s_batt_ema_initialized = false;
static float s_batt_ema_mv = 0.0f;


// Task that notifies battery level every 30 seconds
void battery_notify_task(void *pvParameters) {
    extern uint16_t battery_handle[]; // external declaration    
    while (1) {
        bool usb_connected = is_usb_connected_simple();

        // Battery reading via 100K/100K divider - GPIO automatically from config.h    
        int battery_voltage_mv = get_battery_voltage_mv();  
        
        // Conversion to percentage based on Li-Ion battery range (3.0V-4.2V): 3000 mV = 0%, 4200 mV = 100%
        int battery_percentage = (battery_voltage_mv - 3000) * 100 / (4200 - 3000);        
        if (battery_percentage < 0) battery_percentage = 0;
        if (battery_percentage > 100) battery_percentage = 100;

        if (battery_percentage >= 0) {            
            // Charging when USB connected and not yet full
            display_oled_set_charging(usb_connected && battery_percentage < 98);
            display_oled_set_battery_percent(battery_percentage);

            // Notify only if handle is valid and BLE connection is active
            if (battery_handle[BAS_IDX_BATT_LVL_VAL] != 0) {
                esp_err_t err = battery_notify_level((uint8_t)battery_percentage);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "BAS notify failed: %s", esp_err_to_name(err));
                } 
            }
        }

        // Debug: also send the value in mV via the custom BLE characteristic
        if (battery_voltage_mv > 0) {
            notify_battery_mv(battery_voltage_mv);
        }

        // Debug log
        ESP_LOGI(TAG, "USB %s. Battery (GPIO%d): %dmV (%d%%)", 
            usb_connected ? "connected" : "disconnected", 
            VBAT_GPIO, battery_voltage_mv, battery_percentage);
        vTaskDelay(pdMS_TO_TICKS(10000)); // 30 seconds
    }
}

// Function to start battery notification task
void start_battery_notify_task(void) {
    if (batteryNotifyTaskHandle == NULL) {
        xTaskCreate(battery_notify_task, "battery_notify_task", 3000, NULL, 1, &batteryNotifyTaskHandle);
    }
}


// ADC calibration function (from ESP-IDF 5.5.0 example)
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {        
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
        ESP_LOGI(TAG, "Set calibration scheme version to %s: %s", "Curve Fitting", calibrated ? "Success" : "Failed");
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {        
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
        ESP_LOGI(TAG, "Set calibration scheme version to %s: %s", "Line Fitting", calibrated ? "Success" : "Failed");
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

// ADC initialization for power monitoring (based on ESP-IDF 5.5.0 example)
esp_err_t init_power_monitoring_adc(void) {
    esp_err_t ret = ESP_OK;
    
    // ADC1 one-shot configuration
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Battery channel configuration - automatic from config.h
    adc_oneshot_chan_cfg_t battery_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ret = adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &battery_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Battery ADC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Battery calibration  
    battery_calibrated = adc_calibration_init(ADC_UNIT, BATTERY_ADC_CHANNEL, ADC_ATTEN, &adc_cali_battery_handle);
    return ESP_OK;
}

// USB detection: on ESP32-S3 keep JTAG check; on others use non-blocking battery heuristic
bool is_usb_connected_simple() {
#if CONFIG_IDF_TARGET_ESP32S3
    // Simple 1s cache to avoid spamming logs
    static uint32_t s_last_ts = 0;
    static bool s_last_res = false;
    uint32_t now = xTaskGetTickCount();
    if (s_last_ts != 0 && (now - s_last_ts) < pdMS_TO_TICKS(10000)) {
        return s_last_res;
    }
    // Keep existing method for S3
    bool usb_connected = usb_serial_jtag_is_connected();
    s_last_res = usb_connected;
    s_last_ts = now;
    // ESP_LOGI(TAG, "USB %s", usb_connected ? "CONNECTED" : "DISCONNECTED");
    return usb_connected;
#else    
    // Battery reading via 100K/100K divider - GPIO automatically from config.h    
    int battery_voltage_mv = get_battery_voltage_mv();  
    // Conversion to percentage based on Li-Ion battery range (3.0V-4.2V): 3000 mV = 0%, 4200 mV = 100%
    int battery_percentage = (battery_voltage_mv - 3000) * 100 / (4200 - 3000);        
    return battery_percentage > 100; // Heuristic: >100% means external power (USB) connected
#endif
}



// Returns the battery voltage in mV using the same pipeline as get_battery_percentage
int get_battery_voltage_mv() {
    if (adc_handle == NULL) {
        return -1;
    }

    // Single-shot read (we smooth over time with EMA instead of averaging many samples now)
    int adc_raw = 0;
    if (adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &adc_raw) != ESP_OK) {
        return -1;
    }

    int voltage_mv = 0; // ADC pin voltage in mV after calibration
    if (battery_calibrated && adc_cali_battery_handle) {
        if (adc_cali_raw_to_voltage(adc_cali_battery_handle, adc_raw, &voltage_mv) != ESP_OK) {
            return -1;
        }
    }

    // Apply pin offset, then scale for the divider (100k/100k -> x2 to get battery voltage)
    int32_t calibrated_mv = voltage_mv + (VBAT_ADC_OFFSET_MV / 2); // Offset on the pin in mV
    int measured_batt_mv = (int)calibrated_mv * 2;

    // EMA smoothing across calls
    const float alpha = BATTERY_EMA_ALPHA;
    if (!s_batt_ema_initialized) {
        s_batt_ema_mv = (float)measured_batt_mv;
        s_batt_ema_initialized = true;
    } else {
        s_batt_ema_mv = alpha * (float)measured_batt_mv + (1.0f - alpha) * s_batt_ema_mv;
    }

    int battery_voltage_mv = (int)(s_batt_ema_mv + 0.5f);
    return battery_voltage_mv;
}

// Notify the mV value via BLE through the custom USER_MGMT characteristic
void notify_battery_mv(int mv) {
    extern uint16_t user_mgmt_handle[];
    extern uint16_t user_mgmt_conn_id;
    if (user_mgmt_handle[USER_MGMT_IDX_VAL] == 0) return;

    user_mgmt_payload_t payload = {0};
    payload.cmd = BATTERY_MV;
    payload.index = 0;
    // Insert the mV as a decimal string for debug, or 2-byte binary? Use string for UI compatibility.
    snprintf(payload.data, sizeof(payload.data), "%d", mv);

    esp_err_t err = esp_ble_gatts_send_indicate(
        hidd_le_env.gatt_if,
        user_mgmt_conn_id,
        user_mgmt_handle[USER_MGMT_IDX_VAL],
        sizeof(payload),
        (uint8_t *)&payload,
        false
    );
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "notify_battery_mv failed: %s", esp_err_to_name(err));
    }
}
