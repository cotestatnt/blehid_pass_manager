set(srcs     
    "main.cpp"    
)

set(include_dirs 
    "." 
)

idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES esp_hid mbedtls ble_device oled r503lib user_list
    PRIV_REQUIRES nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)

idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers" "-Wno-deprecated-enum-enum-conversion" APPEND)
