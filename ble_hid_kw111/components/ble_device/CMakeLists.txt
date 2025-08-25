idf_component_register( SRCS 
    "esp_hidd_prf_api.c"     
    "hid_dev.c"     
    "hid_device_usb.c"
    "hid_device_ble.c"    
    "hid_device_prf.c"

    INCLUDE_DIRS "."
    REQUIRES esp_hid user_list main esp_tinyusb
    PRIV_REQUIRES nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable )
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-multichar)