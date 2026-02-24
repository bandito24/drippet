#include "uart.hpp"
#include "config.hpp"
#include "driver/uart.h"
#include "esp32config.hpp"
#include "freertos/projdefs.h"
#include "logger.hpp"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "uart.hpp"
#include <cstdint>
#include <cstdio>
#include <optional>

Esp_Err_t EspUart::init() {
  gpio_num_t esp_tx = static_cast<gpio_num_t>(tx_pin);
  gpio_num_t esp_rx = static_cast<gpio_num_t>(rx_pin);

  uart_config_t uart_config = {
      .baud_rate = BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));

  ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                               esp_tx, // TX
                               esp_rx, // RX
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE, 0, 0, NULL, 0));
  return 0;
}
byte_count EspUart::write_bytes(UartMessage data) const {

  assert(data.data_length <= config::node_hose_count);
  uint8_t data_len = data.data_length * 2; // uint16_t broken up into 2

  uint8_t frame[Protocol::FrameLength] = {Protocol::start_bit, data.address,
                                          static_cast<uint8_t>(data.command),
                                          data_len};
  size_t frame_index = to_index(Protocol::HeaderOrder::HEADER_LENGTH);
  for (size_t i = 0; i < data.data_length; i++) {
    uint16_t byte = data.data[i];
    frame[frame_index++] = byte & 0xFF;
    frame[frame_index++] = (byte >> 8) & 0xFF;
  }
  uint16_t crc = UartFunctions::crc16_modbus(&frame[1], frame_index - 1);
  frame[frame_index++] = crc & 0xFF;
  frame[frame_index++] = (crc >> 8) & 0xFF;
  assert(frame_index < Protocol::FrameLength);
  return uart_write_bytes(UART_PORT, frame, frame_index);
}

void uart_validate_and_enqueue() {

  std::array<uint8_t, 128> buffer{};

  int len = uart_read_bytes(UART_PORT, buffer.data(), buffer.size(),
                            pdMS_TO_TICKS(50));
  if (len == 0)
    return;

  size_t i = 0;
  while (buffer[i] != Protocol::start_bit) {
    i++;
  }
  std::optional<size_t> last_index = calculate_last_index(buffer, i);
  Protocol::Frame frame_seg{};

  size_t frame_length = *last_index - i + 1;
  if (!last_index || frame_length > frame_seg.size()) {
    return;
  }
  std::copy(buffer.begin() + i, buffer.begin() + (*last_index + 1),
            frame_seg.begin());

  if (UartFunctions::validate_frame(buffer) != ParseResult::Ok) {
    return std::nullopt;
  }
  return UartFunctions::reconstruct_uart_message(buffer);
}

EspUart::EspUart() : tx_pin{RS485_TXD_PIN}, rx_pin{RS485_RXD_PIN} {};
EspUart::EspUart(Pin tx, Pin rx) : tx_pin{tx}, rx_pin{rx} {}

uint16_t UartFunctions::crc16_modbus(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];

    for (int j = 0; j < 8; ++j) {
      if (crc & 0x0001)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }

  return crc;
}

ParseResult validate_frame(Protocol::Frame buffer) {

  if (buffer[to_index(MsgIndex::START_BIT)] != Protocol::start_bit) {
    Logger::log_error("Invalid Start Bit"); // Drop it, start process over
                                            // of discovery or water init
                                            // return
    return ParseResult::InvalidStartBit;
  }

  size_t crc_start = to_index(Protocol::HeaderOrder::HEADER_LENGTH) +
                     buffer[to_index(Protocol::HeaderOrder::DATA_LENGTH)];

  uint16_t crc_computed =
      UartFunctions::crc16_modbus(&buffer[1], crc_start - 1);
  uint16_t crc_received = (static_cast<uint16_t>(buffer[crc_start] << 8) |
                           static_cast<uint16_t>(buffer[crc_start + 1]));
  if (crc_computed != crc_received) {
    Logger::log_error("CRC16 MODBUS values do not match"); // Drop it, restart
    return ParseResult::CrcMismatch;
  }
  return ParseResult::Ok;
}

uint16_t UartFunctions::merge_uint8(uint8_t high_bit, uint8_t low_bit) {
  return (static_cast<uint16_t>(high_bit) << 8) |
         static_cast<uint16_t>(low_bit);
}

using Header = Protocol::HeaderOrder;
UartMessage UartFunctions::reconstruct_uart_message(Protocol::Frame buffer) {

  Protocol::FrameDataArray result{};
  size_t data_length = buffer[to_index(Protocol::HeaderOrder::DATA_LENGTH)];
  if (data_length != 0) {
    assert(data_length % 2 == 0);
    for (size_t i = Protocol::DATA_START_INDEX; i < data_length; i = i + 2) {
      result[i - Protocol::DATA_START_INDEX] =
          UartFunctions::merge_uint8(buffer[i], buffer[i + 1]);
    }
  }

  return {.address = buffer[to_index(Header::ADDRESS)],
          .command =
              static_cast<Protocol::Command>(buffer[to_index(Header::COMMAND)]),
          .data = result,
          .data_length = data_length / 2};
}
