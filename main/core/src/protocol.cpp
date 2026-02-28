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
byte_count EspUart::write_bytes(const UartMessage &data) const {

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

FrameIndexes UartFunctions::get_uint8_buffer_indexes(Protocol::Frame frame) {}

// Calculates the exclusive end index (beginning of next frame)
inline std::optional<size_t>
calculate_buffer_frame_end(std::span<const uint8_t> buffer,
                           size_t start_index) {
  assert(buffer[start_index] == Protocol::start_bit);

  size_t data_length_index =
      start_index + to_index(Protocol::HeaderOrder::DATA_LENGTH);
  if (data_length_index >= buffer.size()) {
    return std::nullopt;
  }
  size_t last_index = start_index +
                      to_index(Protocol::HeaderOrder::HEADER_LENGTH) +
                      (buffer[data_length_index]) + 2; // 2 For CRC16
  if (last_index > buffer.size()) {
    return std::nullopt;
  }
  return last_index;
}
size_t get_buffered_rx_length() {
  size_t buffer_length = 0;
  uart_get_buffered_data_len(UART_PORT, (size_t *)&buffer_length);
  return buffer_length;
}

// Sequence Goes:
// 1) uart_read,
// 2) parse_uart_read
// 3) build_uart_message (which uart_task adds to the queue)

// 1)
SizedReadBuffer EspUart::uart_read() {

  SizedReadBuffer buffer{};

  buffer.length = uart_read_bytes(UART_PORT, buffer.content.data(),
                                  buffer.content.size(), pdMS_TO_TICKS(50));
  return buffer;
}
// 2)
std::optional<IndexedFrame> EspUart::parse_uart_read(SizedReadBuffer buffer,
                                                     size_t start_index) {

  if (buffer.length == 0 || start_index >= buffer.length)
    return std::nullopt;

  auto &content = buffer.content;
  size_t i = start_index;
  while (i < buffer.length && (content[i] != Protocol::start_bit)) {
    i++;
  }
  std::optional<size_t> frame_end = calculate_buffer_frame_end(content, i);
  if (!frame_end) {
    return std::nullopt;
  }
  IndexedFrame indexed_frame{};
  auto &frame_seg = indexed_frame.frame;

  size_t frame_length = *frame_end - i;
  if (frame_length > frame_seg.size()) {
    return std::nullopt;
  }
  std::copy(content.begin() + i, content.begin() + (*frame_end),
            frame_seg.begin());

  // BUILD THE INDEXED FRAME
  return indexed_frame;
}

// 3)
std::optional<UartMessage>
EspUart::build_uart_message(Protocol::Frame frame_seg) {

  if (UartFunctions::validate_frame(frame_seg) != ParseResult::Ok) {
    return std::nullopt;
  }
  return UartFunctions::reconstruct_uart_message(frame_seg);
}

// 3a)
ParseResult UartFunctions::validate_frame(Protocol::Frame buffer) {

  if (buffer[to_index(MsgIndex::START_BIT)] != Protocol::start_bit) {
    Logger::log_error("Invalid Start Bit"); // Drop it, start process over
                                            // of discovery or water init
                                            // return
    return ParseResult::InvalidStartBit;
  }

  size_t crc_start = to_index(Protocol::HeaderOrder::HEADER_LENGTH) +
                     buffer[to_index(Protocol::HeaderOrder::DATA_LENGTH)];

  for (size_t i = 0; i < 10; i++) {
    printf("index: %d = %u\n", i, buffer[i]);
  }

  uint16_t crc_computed =
      UartFunctions::crc16_modbus(&buffer[1], crc_start - 1);
  uint16_t crc_received = UartFunctions::merge_uint8(crc_start, crc_start + 1);
  printf("received: %d, calculated: %d", crc_received, crc_computed);
  if (crc_computed != crc_received) {
    Logger::log_error("CRC16 MODBUS values do not match"); // Drop it, restart
    return ParseResult::CrcMismatch;
  }
  return ParseResult::Ok;
}

// 3b)
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

uint16_t
UartFunctions::merge_uint8(uint8_t first_bit,
                           uint8_t second_bit) { // Little Endian Reconstruction
  return (static_cast<uint16_t>(first_bit)) |
         static_cast<uint16_t>(second_bit << 8);
}
