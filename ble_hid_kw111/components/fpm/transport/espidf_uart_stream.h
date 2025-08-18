#pragma once
#include "driver/uart.h"
#include "fpm_transport.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class EspIdfUartStream : public IFpmStream {
public:
    EspIdfUartStream(uart_port_t uart_num,
                     int tx_gpio,
                     int rx_gpio,
                     int baud = 57600,
                     int rx_buf_size = 512,
                     int tx_buf_size = 512);
    ~EspIdfUartStream();

    esp_err_t begin();
    void end();

    int available() override;
    size_t read(uint8_t* buf, size_t len, uint32_t timeout_ms) override;
    size_t write(const uint8_t* data, size_t len) override;
    void flush() override;

private:
    uart_port_t _uart;
    int _tx;
    int _rx;
    int _baud;
    int _rxbuf;
    int _txbuf;
    bool _started;
};