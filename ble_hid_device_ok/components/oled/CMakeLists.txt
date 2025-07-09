idf_component_register(
    SRCS "oled.c" 
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer esp_lcd esp_lvgl_port user_list
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-deprecated-enum-enum-conversion)