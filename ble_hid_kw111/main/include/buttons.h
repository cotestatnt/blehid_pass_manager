#pragma once
#ifndef BUTTONS_H
#define BUTTONS_H

#include "config.h"
#include "oled.h"
#include "user_list.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function to enter deep sleep
void enter_deep_sleep();
void button_task(void *pvParameters);

// Buzzer feedback helpers
void buzzer_feedback_success();
void buzzer_feedback_fail();
void buzzer_feedback_noauth();

#ifdef __cplusplus
}
#endif

#endif // FINGERPRINT_H