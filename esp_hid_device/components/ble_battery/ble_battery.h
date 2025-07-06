#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ble_battery_init(void);
void ble_battery_notify_level(uint8_t level);  // valore da 0 a 100

#ifdef __cplusplus
}
#endif
