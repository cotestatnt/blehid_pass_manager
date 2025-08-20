#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "r503lib.h"
#include "oled.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PASSWORD    0x00000000
#define DEVICE_ADDR 0xFFFFFFFF

// #include "../../main/config.h"
#include "config.h"
static R503Lib fps(FP_UART_PORT, FP_RX, FP_TX, DEVICE_ADDR);

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
