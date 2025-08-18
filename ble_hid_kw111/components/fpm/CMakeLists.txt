idf_component_register(
    SRCS
        "fpm.cpp"
        "transport/espidf_uart_stream.cpp"
    INCLUDE_DIRS
        "include" "."
    REQUIRES
        driver
        freertos
        esp_timer
)