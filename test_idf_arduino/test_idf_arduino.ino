#include <Arduino.h>
#include <esp_log.h>
#include <esp_system.h> 

#include "src/hid/hid_gap.h"
#include "src/hid/hid_keyboard.h"

static const char *TAG = "MAIN";

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    while (!Serial) {
        delay(10); // Wait for serial port to connect
    }
    ESP_LOGI(TAG, "Arduino setup complete.");
    init_ble_keyboard();   
}

void loop() {
    // Main loop does nothing, as the BLE HID keyboard operates in the background
    delay(1000); // Just to prevent busy-waiting
}

