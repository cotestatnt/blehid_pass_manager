#pragma once
#ifndef CONFIG_H
#define CONFIG_H


#include "driver/gpio.h"

#define ESP32C3_TOI 0
#define ESP32C3_SUPERMINI 1
#define ESP32S3_SUPERMINI 2
#define ESP32S3_TEST      3

#define ESP_BOARD ESP32S3_SUPERMINI

// OLED Display configuration
#define OLED_96x16   1
#define OLED_128x32  2

// Configure the OLED display type here
#define OLED_TYPE OLED_96x16

#if ESP_BOARD == ESP32C3_TOI
    
    #define BUTTON_UP            6
    #define BUTTON_DOWN          7
    #define BUZZER_GPIO          10
    
    #define FP_TOUCH             4
    #define FP_TX                9
    #define FP_RX                8
    #define FP_ACTIVATE          5
    #define PIN_NUM_SDA          18
    #define PIN_NUM_SCL          19
    #define PIN_NUM_RST          -1

    #define VBAT_GPIO            2  
    #define FP_UART_PORT         UART_NUM_1
    #define BATTERY_ADC_CHANNEL  ADC_CHANNEL_2
    #define ADC_UNIT             ADC_UNIT_1

#elif ESP_BOARD == ESP32C3_SUPERMINI
   
    #define BUTTON_UP            3
    #define BUTTON_DOWN          10
    #define BUZZER_GPIO          1
    #define FP_UART_PORT         UART_NUM_1
    #define FP_TOUCH             0
    #define FP_TX                21
    #define FP_RX                20
    #define FP_ACTIVATE          7
    #define PIN_NUM_SDA          8
    #define PIN_NUM_SCL          9
    #define PIN_NUM_RST          -1    

    #define VBAT_GPIO            4  
    #define BATTERY_ADC_CHANNEL  ADC_CHANNEL_4
    #define ADC_UNIT             ADC_UNIT_1

     // Offset sul pin in mV (dovuto alla caduta di tensione sul diodo schottky)
    #define VBAT_ADC_OFFSET_MV   284     

#elif ESP_BOARD == ESP32S3_SUPERMINI
    #define FP_UART_PORT         UART_NUM_1
    #define BUZZER_GPIO          10


    #define BUTTON_UP            12
    #define BUTTON_DOWN          4

    #define FP_TOUCH             13
    #define FP_TX                6
    #define FP_RX                5
    #define FP_ACTIVATE          1

    #define PIN_NUM_SDA          2
    #define PIN_NUM_SCL          3
    #define PIN_NUM_RST          -1
    
    // ESP32-S3, GPIO 11 corrisponde ad ADC2_CH0
    #define VBAT_GPIO            11  
    #define BATTERY_ADC_CHANNEL ADC_CHANNEL_0
    #define ADC_UNIT            ADC_UNIT_2    
    #define VBAT_ADC_OFFSET_MV   100    // Offset sul pin in mV

#elif ESP_BOARD == ESP32S3_TEST
    #define FP_UART_PORT         UART_NUM_1
  
    #define BUZZER_GPIO          11        
    #define BUTTON_UP            12
    #define BUTTON_DOWN          13

    #define FP_ACTIVATE          10
    #define FP_TX                6
    #define FP_RX                4
    #define FP_TOUCH             5

    #define PIN_NUM_SDA          1
    #define PIN_NUM_SCL          2
    #define PIN_NUM_RST          -1
    
    // ESP32-S3, GPIO 9 corrisponde ad ADC1_CH8 (ADC_CHANNEL_8)    
    #define VBAT_GPIO            9 
    #define BATTERY_ADC_CHANNEL ADC_CHANNEL_8
    // Default calibrazione neutra (override se necessario)
    #define VBAT_ADC_SCALE_NUM   1000
    #define VBAT_ADC_SCALE_DEN   1000
    #define VBAT_ADC_OFFSET_MV   0
    
#else
    #error "Unsupported ESP board configuration"    
#endif


// Se non definiti nel blocco della board, usa valori neutri
#ifndef VBAT_ADC_SCALE_NUM
#define VBAT_ADC_SCALE_NUM 1000
#endif
#ifndef VBAT_ADC_SCALE_DEN
#define VBAT_ADC_SCALE_DEN 1000
#endif
#ifndef VBAT_ADC_OFFSET_MV
#define VBAT_ADC_OFFSET_MV 0
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


#define I2C_BUS_PORT  0
#define LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define I2C_HW_ADDR           0x3C



#if OLED_TYPE == OLED_96x16
#define LCD_H_RES 96
#define LCD_V_RES 16
#else
#define LCD_H_RES 128
#define LCD_V_RES 32
#endif


#endif // CONFIG_H