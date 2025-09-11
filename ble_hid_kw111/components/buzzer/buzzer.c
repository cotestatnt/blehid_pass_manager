#include "buzzer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "config.h"  // Provides BUZZER_GPIO
#include "buzzer.h"

static const char *TAG = "BUZZ";

// Internal queue and task handle
static QueueHandle_t s_buzz_queue = NULL;
static TaskHandle_t  s_buzz_task  = NULL;

static inline void buzzer_set(int on) {
    gpio_set_level((gpio_num_t)BUZZER_GPIO, on ? 1 : 0);
}

static void buzzer_beep_ms(uint32_t ms) {
    buzzer_set(1);
    vTaskDelay(pdMS_TO_TICKS(ms));
    buzzer_set(0);
}

static void buzzer_task(void *arg) {
    buzzer_event_t ev;
    for(;;) {
        if (xQueueReceive(s_buzz_queue, &ev, portMAX_DELAY) == pdTRUE) {
            switch (ev) {
                case BUZZ_EVENT_STARTUP:
                    buzzer_beep_ms(100);
                    break;
                case BUZZ_EVENT_ENROLL:
                    buzzer_beep_ms(50);
                    break;
                case BUZZ_EVENT_SUCCESS:
                    buzzer_beep_ms(350);
                    break;
                case BUZZ_EVENT_LONG:
                    buzzer_beep_ms(800);
                    break;
                case BUZZ_EVENT_FAIL:
                    buzzer_beep_ms(40);
                    vTaskDelay(pdMS_TO_TICKS(60));
                    buzzer_beep_ms(40);
                    break;
                case BUZZ_EVENT_NOAUTH:
                    vTaskDelay(pdMS_TO_TICKS(50));
                    buzzer_beep_ms(50);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    buzzer_beep_ms(50);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    buzzer_beep_ms(50);
                    break;
                default:
                    break;
            }
        }
    }
}

esp_err_t buzzer_init(void) {
    if (s_buzz_task) {
        return ESP_OK; // already initialized
    }

    // Configure GPIO as output
    gpio_config_t buzzer_conf = {0};
    buzzer_conf.intr_type = GPIO_INTR_DISABLE;
    buzzer_conf.mode = GPIO_MODE_OUTPUT;
    buzzer_conf.pin_bit_mask = (1ULL << (gpio_num_t)BUZZER_GPIO);
    buzzer_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    buzzer_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    esp_err_t err = gpio_config(&buzzer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(err));
        return err;
    }
    buzzer_set(0);

    s_buzz_queue = xQueueCreate(8, sizeof(buzzer_event_t));
    if (!s_buzz_queue) {
        ESP_LOGE(TAG, "Failed to create buzzer queue");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreatePinnedToCore(buzzer_task, "buzzer_task", 2048, NULL, 3, &s_buzz_task, tskNO_AFFINITY);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create buzzer task");
        vQueueDelete(s_buzz_queue);
        s_buzz_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    // Startup beep (non-blocking enqueue)
    buzzer_post_event(BUZZ_EVENT_STARTUP, 0);
    ESP_LOGI(TAG, "Buzzer initialized (GPIO=%d)", BUZZER_GPIO);
    return ESP_OK;
}

bool buzzer_post_event(buzzer_event_t ev, TickType_t ticks_to_wait) {
    if (!s_buzz_queue) return false;
    return xQueueSend(s_buzz_queue, &ev, ticks_to_wait) == pdTRUE;
}

// Convenience wrappers
void buzzer_feedback_success(void) { buzzer_post_event(BUZZ_EVENT_SUCCESS, 0); }
void buzzer_feedback_fail(void)    { buzzer_post_event(BUZZ_EVENT_FAIL, 0); }
void buzzer_feedback_noauth(void)  { buzzer_post_event(BUZZ_EVENT_NOAUTH, 0); }
void buzzer_feedback_lift(void)    { buzzer_post_event(BUZZ_EVENT_ENROLL, 0); }
void buzzer_feedback_long(void)   { buzzer_post_event(BUZZ_EVENT_LONG, 0); }
