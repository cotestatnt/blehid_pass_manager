idf_component_register(
    SRCS "hid_keyboard.c" "hid_gap.c"
    INCLUDE_DIRS "."
    REQUIRES esp_hid ble_battery
)
