idf_component_register( SRCS 
    "hid_device_usb.c"
    "esp_hidd_prf_api.c" 
    "hid_dev.c" 
    "hid_device_le_prf.c" 
    "ble_device_main.c"
    "ble_userlist_auth.c"
    INCLUDE_DIRS "."
    REQUIRES esp_hid user_list main esp_tinyusb
    PRIV_REQUIRES nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable )
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-multichar)