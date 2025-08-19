#pragma once
#ifndef CONFIG_H
#define CONFIG_H


#include "driver/gpio.h"

#define ESP32C3_TOI 0
#define ESP32C3_SUPERMINI 1
#define ESP32S3_SUPERMINI 2
#define ESP32S3_TEST      3

#define ESP_BOARD ESP32S3_TEST

#if ESP_BOARD == ESP32C3_TOI
    #define VBAT_GPIO            1
    #define BUTTON_UP            6
    #define BUTTON_DOWN          7
    #define BUZZER_GPIO          10
    #define FP_UART_PORT         UART_NUM_1
    #define FP_TOUCH             4
    #define FP_TX                9
    #define FP_RX                8
    #define FP_ACTIVATE          5
    #define PIN_NUM_SDA          18
    #define PIN_NUM_SCL          19
    #define PIN_NUM_RST          -1

#elif ESP_BOARD == ESP32C3_SUPERMINI
    #define VBAT_GPIO            2  
    #define BUTTON_UP            3
    #define BUTTON_DOWN          10
    #define BUZZER_GPIO          1
    #define FP_UART_PORT         UART_NUM_1
    #define FP_TOUCH             6
    #define FP_TX                21
    #define FP_RX                20
    #define FP_ACTIVATE          7
    #define PIN_NUM_SDA          8
    #define PIN_NUM_SCL          9
    #define PIN_NUM_RST          -1

#elif ESP_BOARD == ESP32S3_SUPERMINI
    #define FP_UART_PORT         UART_NUM_1

    #define BUZZER_GPIO          10
    #define VBAT_GPIO            11  
    #define BUTTON_UP            12
    #define BUTTON_DOWN          4
    #define FP_TOUCH             13
    #define FP_TX                6
    #define FP_RX                5
    #define FP_ACTIVATE          1  
    #define PIN_NUM_SDA          2
    #define PIN_NUM_SCL          3
    #define PIN_NUM_RST          -1

#elif ESP_BOARD == ESP32S3_TEST
    #define FP_UART_PORT         UART_NUM_1
  
    #define BUTTON_UP            7
    #define BUTTON_DOWN          5

    #define VBAT_GPIO            8  
    #define BUZZER_GPIO          9

    #define FP_ACTIVATE          10
    #define FP_TOUCH             6
    
    #define FP_RX                3
    #define FP_TX                4

    #define PIN_NUM_SDA          1
    #define PIN_NUM_SCL          2
    #define PIN_NUM_RST          -1

#else
    #error "Unsupported ESP board configuration"    
#endif


#define R503_FINGERPRINT 1
#define ZW111_FINGERPRINT 2

#define FP_SENSOR_TYPE ZW111_FINGERPRINT

#if FP_SENSOR_TYPE == ZW111_FINGERPRINT
    #define PULLDOWN_TYPE GPIO_PULLDOWN_ENABLE
    #define ACTIVE_LEVEL 1 // ZW111 touch is active high
#elif FP_SENSOR_TYPE == R503_FINGERPRINT
    #define PULLDOWN_TYPE GPIO_PULLDOWN_DISABLE
    #define ACTIVE_LEVEL 0 // R503 touch is active low
#endif


#endif // CONFIG_H