#pragma once
#ifndef BATTERY_H
#define BATTERY_H

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "config.h"


#ifdef __cplusplus
extern "C" {
#endif

// Function to enter deep sleep
void enter_deep_sleep();

// Funzione di calibrazione ADC (dall'esempio ESP-IDF 5.5.0)
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

// ADC initialization for power monitoring (based on ESP-IDF 5.5.0 example)
esp_err_t init_power_monitoring_adc(void);

// USB detection tramite USB Serial JTAG
bool is_usb_connected_simple();

// Lettura batteria in millivolt
int get_battery_voltage_mv();

// Avvia il task di notifica BLE del livello batteria
void start_battery_notify_task(void);

// Notifica BLE del valore in mV (sul canale custom di gestione utenti)
void notify_battery_mv(int mv);

#ifdef __cplusplus
}
#endif

#endif // FINGERPRINT_H