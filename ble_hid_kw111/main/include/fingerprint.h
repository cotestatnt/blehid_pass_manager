#pragma once
#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include "fpm.h"
#include "transport/espidf_uart_stream.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

void fingerprint_task(void *pvParameters);

bool enrollFinger();
bool clearFingerprintDB();
int searchDatabase();

#ifdef __cplusplus
}
#endif

#endif // FINGERPRINT_H