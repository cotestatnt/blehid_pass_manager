#include "esp_log.h"
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

#include "battery.h"
#include "config.h"


static const char *TAG = "BAT";
static TaskHandle_t batteryNotifyTaskHandle = NULL;

// ADC for power monitoring (based on ESP-IDF 5.5.0 example)
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_battery_handle = NULL;  // Used for battery voltage
static bool battery_calibrated = false;

#define ADC_ATTEN           ADC_ATTEN_DB_12
#define ADC_BITWIDTH        ADC_BITWIDTH_DEFAULT

#if CONFIG_IDF_TARGET_ESP32C3
// Fast helper to read battery voltage in mV (non-blocking, few samples)
static int read_battery_mv(void) {
    if (adc_handle == NULL) {
        return -1;
    }
    const int num_samples = 3; // keep it quick
    int sum = 0, n = 0;
    for (int i = 0; i < num_samples; ++i) {
        int raw = 0;
        if (adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &raw) == ESP_OK) {
            sum += raw;
            n++;
        }
    }
    if (!n) return -1;
    int avg = sum / n;
    int mv = 0;
    if (battery_calibrated && adc_cali_battery_handle) {
        if (adc_cali_raw_to_voltage(adc_cali_battery_handle, avg, &mv) != ESP_OK) return -1;
    } else {
        // Approximate conversion when calibration not available
        mv = (avg * 3100) / 4095;
    }
    return mv * 2; // divider 100k/100k => real Vbat
}
#endif

// Task that notifies battery level every 30 seconds
void battery_notify_task(void *pvParameters) {
    extern uint16_t battery_handle[]; // external declaration    
    while (1) {
        int battery_percentage = get_battery_percentage();
        if (battery_percentage >= 0) {

            // Notify only if handle is valid and BLE connection is active
            if (battery_handle[BAS_IDX_BATT_LVL_VAL] != 0) {

                ///////////////////////   TEST   ///////////////////////////
                // Aggiunge un po' di rumore per evitare notifiche identiche
                ////////////////////////////////////////////////////////////
                battery_percentage = battery_percentage - esp_random() % 10;
                ////////////////////////////////////////////////////////////
                ///////////////////////   TEST   ///////////////////////////

                esp_err_t err = battery_notify_level((uint8_t)battery_percentage);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "BAS notify failed: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGI(TAG, "Battery level notified: %d%%", battery_percentage);
                }
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
bool is_usb_connected_simple(void) {
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
    ESP_LOGI(TAG, "USB %s", usb_connected ? "CONNECTED" : "DISCONNECTED");
    return usb_connected;
#else
    // Heuristic for C3/others without VBUS sense
    const int VBAT_PRESENCE_MV = 4100;    // >= 4.10V likely means USB/charger present
    const int RISE_THRESHOLD_MV = 20;     // rising by >=20 mV in ~30s indicates charging
    const uint32_t SLOPE_WINDOW_MS = 30000; // compare every ~30s (non-blocking)

    // 1s cache to reduce ADC reads and log noise
    static uint32_t s_last_ts = 0;
    static bool s_last_res = true;
    uint32_t now = xTaskGetTickCount();
    if (s_last_ts != 0 && (now - s_last_ts) < pdMS_TO_TICKS(10000)) {
        return s_last_res;
    }

    static int prev_mv = -1;
    static uint32_t prev_ts = 0;

    int mv = read_battery_mv();
    if (mv < 0) {
        // If ADC not ready, fall back to JTAG state (may be false-negative on C3)
        bool jtag = usb_serial_jtag_is_connected();
        s_last_res = jtag;
        s_last_ts = now;
        ESP_LOGI(TAG, "USB heuristic fallback: %s (ADC fail)", jtag ? "CONNECTED" : "DISCONNECTED");
        return jtag;
    }
    if (mv >= VBAT_PRESENCE_MV) {
        s_last_res = true;
        s_last_ts = now;
        ESP_LOGI(TAG, "USB heuristic: CONNECTED (Vbat=%d mV >= %d mV)", mv, VBAT_PRESENCE_MV);
        // update history lazily
        if (prev_mv < 0 || (now - prev_ts) >= pdMS_TO_TICKS(SLOPE_WINDOW_MS)) {
            prev_mv = mv; prev_ts = now;
        }
        return s_last_res;
    }

    // Non-blocking slope check: only evaluate when window elapsed
    bool rising = false;
    if (prev_mv >= 0 && (now - prev_ts) >= pdMS_TO_TICKS(SLOPE_WINDOW_MS)) {
        rising = (mv - prev_mv) >= RISE_THRESHOLD_MV;
        prev_mv = mv; prev_ts = now;
    } else if (prev_mv < 0) {
        // initialize history on first call
        prev_mv = mv; prev_ts = now;
    }

    if (rising) {
        s_last_res = true;
        s_last_ts = now;
        ESP_LOGI(TAG, "USB heuristic: CONNECTED (Vbat rising >=%d mV)", RISE_THRESHOLD_MV);
        return s_last_res;
    }

    s_last_res = false;
    s_last_ts = now;
    ESP_LOGI(TAG, "USB heuristic: DISCONNECTED (Vbat=%d mV)", mv);
    return s_last_res;
#endif
}



// Lettura batteria tramite partitore 100K/100K - GPIO automatico da config.h
int get_battery_percentage(void) {
    if (adc_handle == NULL) {
        return -1;
    }
    
    // Stabilizzazione: effettua multiple letture e calcola la media
    const int num_samples = 10;
    int adc_sum = 0;
    int valid_samples = 0;
    
    for (int i = 0; i < num_samples; i++) {
        int adc_raw = 0;
        esp_err_t ret = adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &adc_raw);
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
    if (battery_calibrated && adc_cali_battery_handle) {        
        esp_err_t ret = adc_cali_raw_to_voltage(adc_cali_battery_handle, adc_avg, &voltage_mv);
        if (ret != ESP_OK) {
            return -1;
        }
    } else {
        voltage_mv = (adc_avg * 3050) / 4095;
    }
    
    // La tensione misurata è già quella del partitore (Vbat/2)
    // Per ottenere la tensione reale della batteria, moltiplichiamo per 2
    int battery_voltage_mv = voltage_mv * 2;
    
    // Conversione a percentuale basata su range batteria Li-Ion (3.0V-4.2V)
    // 3000mV = 0%, 4200mV = 100%
    int percentage = ((battery_voltage_mv - 3000) * 100) / (4200 - 3000);
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    ESP_LOGI(TAG, "Battery (GPIO%d): ADC=%dmV, Real=%dmV, %d%%", VBAT_GPIO, voltage_mv, battery_voltage_mv, percentage);
    return percentage;
}