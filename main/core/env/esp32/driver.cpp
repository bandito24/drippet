#include "driver.hpp"
#include "driver/uart.h"
#include "esp32config.hpp"
#include "freertos/projdefs.h"
#include "logger.hpp"
#include "protocol.hpp"
#include <cstdio>

Esp_Err_t UartDriver::init() {
  gpio_num_t esp_tx = static_cast<gpio_num_t>(tx_pin);
  gpio_num_t esp_rx = static_cast<gpio_num_t>(rx_pin);

  uart_config_t uart_config{}; // Zero-initialize everything
  uart_config.baud_rate = BAUD_RATE;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.rx_flow_ctrl_thresh = 122;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));

  ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                               esp_tx, // TX
                               esp_rx, // RX
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE, 0, 0, NULL, 0));
  return 0;
}
byte_count UartDriver::send(Protocol::Frame frame, size_t frame_length) const {

  return uart_write_bytes(UART_PORT, frame.data(), frame_length);
}

SizedReadBuffer UartDriver::receive() {
  SizedReadBuffer buffer{};

  if (get_buffered_rx_length() != 0) {
    buffer.length = uart_read_bytes(UART_PORT, buffer.content.data(),
                                    buffer.content.size(), pdMS_TO_TICKS(50));
  }

  return buffer;
}
UartDriver::UartDriver() : tx_pin{RS485_TXD_PIN}, rx_pin{RS485_RXD_PIN} {};
UartDriver::UartDriver(Pin tx, Pin rx) : tx_pin{tx}, rx_pin{rx} {}

size_t UartDriver::get_buffered_rx_length() {
  size_t buffer_length = 0;
  uart_get_buffered_data_len(UART_PORT, (size_t *)&buffer_length);
  return buffer_length;
}
