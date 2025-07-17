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

#define UART_PORT   UART_NUM_1
#define TOUCH_GPIO  4
#define TX_GPIO     9
#define RX_GPIO     8
#define PASSWORD    0x00000000
#define DEVICE_ADDR 0xFFFFFFFF

static R503Lib fps(UART_PORT, RX_GPIO, TX_GPIO, DEVICE_ADDR);

void fingerprint_task_start(void);

void enrollFinger();
void clearFingerprintDB();
bool searchFinger();

extern uint16_t num_templates;

#ifdef __cplusplus
}
#endif
