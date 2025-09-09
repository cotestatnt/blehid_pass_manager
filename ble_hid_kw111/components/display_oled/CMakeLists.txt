idf_component_register(
    SRCS "display_oled.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl esp_lcd esp_driver_i2c heap main
)

