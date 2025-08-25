#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
#include "hid_device_prf.h"
}


#if CONFIG_IDF_TARGET_ESP32S3
#include "driver/rtc_io.h"
#endif

#include "battery.h"
#include "config.h"


static const char *TAG = "BAT";
static TaskHandle_t batteryNotifyTaskHandle = NULL;

// ADC for power monitoring (based on ESP-IDF 5.5.0 example)
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_vbus_handle = NULL;
static adc_cali_handle_t adc1_cali_battery_handle = NULL;  // Used for 3.3V rail
static bool vbus_calibrated = false;
static bool battery_calibrated = false; 

#define VBUS_ADC_CHANNEL    ADC_CHANNEL_2           // GPIO 3 - VBUS con divisore integrato dev board
#define ADC_ATTEN           ADC_ATTEN_DB_12
#define ADC_BITWIDTH        ADC_BITWIDTH_DEFAULT

// Task that notifies battery level every 30 seconds
void battery_notify_task(void *pvParameters) {
    extern uint16_t battery_handle[]; // external declaration    
    while (1) {
        int battery_percentage = get_battery_percentage();
        if (battery_percentage >= 0) {
            // Notify only if handle is valid and BLE connection is active
            if (battery_handle[BAS_IDX_BATT_LVL_VAL] != 0) {
                battery_notify_level((uint8_t)battery_percentage);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(30000)); // 30 seconds
    }
}

// Function to start battery notification task
void start_battery_notify_task(void) {
    if (batteryNotifyTaskHandle == NULL) {
        xTaskCreate(battery_notify_task, "battery_notify_task", 3000, NULL, 1, &batteryNotifyTaskHandle);
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


// ADC calibration function (from ESP-IDF 5.5.0 example)
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
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

// ADC initialization for power monitoring (based on ESP-IDF 5.5.0 example)
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

    // VBUS channel configuration (GPIO 3) - with integrated divider on dev board
    adc_oneshot_chan_cfg_t vbus_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ret = adc_oneshot_config_channel(adc1_handle, VBUS_ADC_CHANNEL, &vbus_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "VBUS ADC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Battery channel configuration - automatic from config.h
    adc_oneshot_chan_cfg_t battery_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ret = adc_oneshot_config_channel(adc1_handle, BATTERY_ADC_CHANNEL, &battery_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Battery ADC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // VBUS calibration
    vbus_calibrated = adc_calibration_init(ADC_UNIT_1, VBUS_ADC_CHANNEL, ADC_ATTEN, &adc1_cali_vbus_handle);
    
    // Battery calibration  
    battery_calibrated = adc_calibration_init(ADC_UNIT_1, BATTERY_ADC_CHANNEL, ADC_ATTEN, &adc1_cali_battery_handle);

    return ESP_OK;
}

// VBUS voltage reading for USB detection
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
        
        // Empirical compensation factor based on tests:
        // We should read ~5000mV but we read 6292mV with 2x compensation
        // Correct factor = 5000 / 6292 * 2 = 1.59
        voltage_mv = (voltage_mv * 159) / 100;  // Factor 1.59 instead of 2.0
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
    ESP_LOGD(TAG, "VBUS voltage: %dmV, USB %s", vbus_mv, usb_connected ? "CONNECTED" : "DISCONNECTED");
    return usb_connected;
}

// Lettura batteria tramite partitore 100K/100K - GPIO automatico da config.h
int get_battery_percentage(void) {
    if (adc1_handle == NULL) {
        return -1;
    }
    
    // Stabilizzazione: effettua multiple letture e calcola la media
    const int num_samples = 10;
    int adc_sum = 0;
    int valid_samples = 0;
    
    for (int i = 0; i < num_samples; i++) {
        int adc_raw = 0;
        esp_err_t ret = adc_oneshot_read(adc1_handle, BATTERY_ADC_CHANNEL, &adc_raw);
        if (ret == ESP_OK) {
            adc_sum += adc_raw;
            valid_samples++;
        }
        // Piccola pausa tra le letture per stabilizzare
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    if (valid_samples == 0) {
        return -1;
    }
    
    int adc_avg = adc_sum / valid_samples;
    int voltage_mv = 0;
    if (battery_calibrated && adc1_cali_battery_handle) {        
        esp_err_t ret = adc_cali_raw_to_voltage(adc1_cali_battery_handle, adc_avg, &voltage_mv);
        if (ret != ESP_OK) {
            return -1;
        }
    } else {
        voltage_mv = (adc_avg * 3100) / 4095;
    }
    
    // La tensione misurata è già quella del partitore (Vbat/2)
    // Per ottenere la tensione reale della batteria, moltiplichiamo per 2
    int battery_voltage_mv = voltage_mv * 2;
    
    // Conversione a percentuale basata su range batteria Li-Ion (3.0V-4.2V)
    // 3000mV = 0%, 4200mV = 100%
    int percentage = ((battery_voltage_mv - 3000) * 100) / (4200 - 3000);
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    ESP_LOGD(TAG, "Battery (GPIO%d): ADC=%dmV, Real=%dmV, %d%%", VBAT_GPIO, voltage_mv, battery_voltage_mv, percentage);
    return percentage;
}