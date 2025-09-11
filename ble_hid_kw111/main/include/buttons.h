#pragma once
#ifndef BUTTONS_H
#define BUTTONS_H

#include "config.h"
#include "display_oled.h"
#include "user_list.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function to enter deep sleep
void enter_deep_sleep();
void button_task(void *pvParameters);


#ifdef __cplusplus
}
#endif

#endif // FINGERPRINT_H