#pragma once
/* Public API for buzzer feedback component */

#include "esp_err.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BUZZ_EVENT_STARTUP = 0,
    BUZZ_EVENT_SUCCESS,
    BUZZ_EVENT_LONG,
    BUZZ_EVENT_FAIL,
    BUZZ_EVENT_NOAUTH,
    BUZZ_EVENT_ENROLL,

} buzzer_event_t;

// Initialize buzzer GPIO, queue and worker task. Safe to call once.
esp_err_t buzzer_init(void);

// Post a raw event (non-blocking). Returns true if queued.
bool buzzer_post_event(buzzer_event_t ev, TickType_t ticks_to_wait);

// Convenience helpers (non-blocking post wrappers)
void buzzer_feedback_success(void);
void buzzer_feedback_fail(void);
void buzzer_feedback_noauth(void);
void buzzer_feedback_lift(void);
void buzzer_feedback_long(void);

#ifdef __cplusplus
}
#endif
