set(_srcs
    "esp_hidd_prf_api.c"
    "hid_dev.c"
    "hid_device_ble.c"
    "hid_device_prf.c"
)

set(_requires esp_hid user_list main)

if(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND _srcs "hid_device_usb.c")
    list(APPEND _requires esp_tinyusb)
else()
    # Provide a stub so symbols are available on non-esp32s3 targets
    list(APPEND _srcs "hid_device_usb_stub.c")
endif()

idf_component_register(
    SRCS ${_srcs}
    INCLUDE_DIRS "."
    REQUIRES ${_requires}
    PRIV_REQUIRES nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable )
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-multichar)