idf_component_register(
  SRCS "ble_battery.c"
  INCLUDE_DIRS "."
  REQUIRES driver esp_hid esp_adc
)
