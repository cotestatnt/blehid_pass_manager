idf_component_register(
    SRCS "buttons.cpp" "battery.cpp" "main.cpp" "fingerprint.cpp" 
    INCLUDE_DIRS "." "include"
    REQUIRES esp_hid mbedtls ble_device display_oled fpm user_list hal
    PRIV_REQUIRES nvs_flash esp_adc
)

# Rendi config.h disponibile a tutti i componenti
target_include_directories(${COMPONENT_LIB} PUBLIC .)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)

idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers" "-Wno-deprecated-enum-enum-conversion" APPEND)
