idf_component_register(
    SRCS "kw111lib.cpp" "fingerprint.cpp"
    INCLUDE_DIRS "." 
    REQUIRES driver esp_timer oled ble_device user_list main
)

