idf_component_register(
    SRCS "r503lib.cpp" "r503packet.cpp" "r503.cpp"
    INCLUDE_DIRS "." 
    REQUIRES driver esp_timer oled ble_device user_list main
)

