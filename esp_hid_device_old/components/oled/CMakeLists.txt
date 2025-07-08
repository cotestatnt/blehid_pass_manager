idf_component_register(
    SRCS "oled.c" 
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer esp_lcd esp_lvgl_port main
)
