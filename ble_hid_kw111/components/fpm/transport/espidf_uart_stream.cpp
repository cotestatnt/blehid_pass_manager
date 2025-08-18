#include "espidf_uart_stream.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char* TAG = "FPM_UART";

EspIdfUartStream::EspIdfUartStream(uart_port_t uart_num,
                                   int tx_gpio,
                                   int rx_gpio,
                                   int baud,
                                   int rx_buf_size,
                                   int tx_buf_size)
: _uart(uart_num),
  _tx(tx_gpio),
  _rx(rx_gpio),
  _baud(baud),
  _rxbuf(rx_buf_size),
  _txbuf(tx_buf_size),
  _started(false) {}

EspIdfUartStream::~EspIdfUartStream() {
    end();
}

esp_err_t EspIdfUartStream::begin() {
    if (_started) return ESP_OK;

    uart_config_t cfg = {};
    cfg.baud_rate = _baud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity    = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    cfg.source_clk = UART_SCLK_DEFAULT;
#endif

    ESP_ERROR_CHECK(uart_driver_install(_uart, _rxbuf, _txbuf, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(_uart, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(_uart, _tx, _rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    _started = true;
    ESP_LOGI(TAG, "UART%u started at %d bps", (unsigned)_uart, _baud);
    return ESP_OK;
}

void EspIdfUartStream::end() {
    if (_started) {
        uart_driver_delete(_uart);
        _started = false;
    }
}

int EspIdfUartStream::available() {
    if (!_started) return 0;
    size_t len = 0;
    if (uart_get_buffered_data_len(_uart, &len) == ESP_OK) {
        return (int)len;
    }
    return 0;
}

size_t EspIdfUartStream::read(uint8_t* buf, size_t len, uint32_t timeout_ms) {
    if (!_started || buf == nullptr || len == 0) return 0;
    TickType_t to_ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
    int r = uart_read_bytes(_uart, buf, len, to_ticks);
    return (r < 0) ? 0 : (size_t)r;
}

size_t EspIdfUartStream::write(const uint8_t* data, size_t len) {
    if (!_started || data == nullptr || len == 0) return 0;
    int w = uart_write_bytes(_uart, (const char*)data, len);
    return (w < 0) ? 0 : (size_t)w;
}

void EspIdfUartStream::flush() {
    if (!_started) return;
    // attende che TX sia svuotato
    (void)uart_wait_tx_done(_uart, pdMS_TO_TICKS(1000));
}