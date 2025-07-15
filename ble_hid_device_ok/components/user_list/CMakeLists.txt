idf_component_register(
    SRCS "user_list.c"
    INCLUDE_DIRS "."
    REQUIRES mbedtls ble_device r503lib oled
    PRIV_REQUIRES nvs_flash
)
