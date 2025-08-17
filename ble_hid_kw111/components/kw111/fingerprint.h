#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "kw111lib.h"
#include "oled.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FP_PASSWORD 0x00000000
#define FP_ADDRESS 0xFFFFFFFF

#include "config.h"
static FingerprintSensor fps(FP_UART_PORT, FP_RX, FP_TX, FP_ADDRESS, FP_PASSWORD);

void fingerprint_task_start(void);

void enrollFinger();
void clearFingerprintDB();
int searchFinger();

void printFingerprintTable();

extern uint16_t num_templates;
extern bool enrolling_in_progress;

#ifdef __cplusplus
}
#endif
