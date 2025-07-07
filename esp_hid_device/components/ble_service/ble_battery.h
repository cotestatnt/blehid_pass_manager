#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ble_battery_init(void);
void ble_battery_set_level(uint8_t percent);
void ble_battery_notify_level(uint8_t percent);

#ifdef __cplusplus
}
#endif
