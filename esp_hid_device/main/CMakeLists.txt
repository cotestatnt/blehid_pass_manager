set(srcs 
    "shared_data.c"
    "main.cpp"
)

set(include_dirs 
    "." 
)

idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES esp_hid mbedtls hid_keyboard oled r503lib ble_battery
    PRIV_REQUIRES nvs_flash
)

idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers" APPEND)

