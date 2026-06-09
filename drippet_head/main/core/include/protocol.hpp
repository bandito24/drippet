#pragma once
#include "config.hpp"
#include "driver.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdint.h>

struct SizedFrameBuffer {
  Protocol::Frame frame;
  size_t content_length;
};

constexpr size_t to_index(Protocol::HeaderOrder h) noexcept {
  return static_cast<size_t>(h);
}
struct FrameDataResult {
  Protocol::FrameDataArray data;
  size_t data_length;
};
struct UartHeader {
  uint8_t address;
  Protocol::Command command;
  UartHeader(uint8_t _address, Protocol::Command _command)
      : address{_address}, command{_command} {}
};
constexpr uint8_t ADDR_UNSET = config::max_nodes;

struct UartMessage {
  uint8_t address{};
  Protocol::Command command{};
  Protocol::FrameDataArray data{}; // Represents the max uint16_t data size
  size_t data_length{};
  bool operator==(const UartMessage &msg) const {
    return this->address == msg.address &&
           this->data_length == msg.data_length &&
           this->command == msg.command &&
           std::equal(this->data.begin(),
                      (this->data.begin() + this->data_length),
                      msg.data.begin());
  }
};

struct FrameIndexes {
  bool has_data = false;
  size_t cdc_start;
  size_t length;
};
struct IndexedFrame {
  FrameIndexes i;
  Protocol::Frame frame;
};

class UartProtocol {
public:
  std::optional<IndexedFrame> parse_uart_read(const SizedReadBuffer &buffer,
                                              size_t start_index);
  std::optional<UartMessage>
  build_uart_message(const Protocol::Frame &frame_seg);
  byte_count write_bytes(const SizedFrameBuffer &msg) const;
  SizedReadBuffer receive_bytes() const;
  SizedFrameBuffer prepare_bytes(const UartMessage &data) const;
  UartProtocol(Driver &driver);

private:
  Driver &driver;
};

enum MsgType { DISCOVERY, ADDRESSING };
enum MsgCode { N0_DATA = 200 };

struct IncomingMsg {
  MsgType type;
  uint8_t data;
};
struct OutgoingMsg {
  MsgType type;
};

enum class ParseResult { Ok, CrcMismatch, InvalidStartBit, InvalidLength };

constexpr size_t dataIndex = to_index(Protocol::HeaderOrder::HEADER_LENGTH);
using MsgIndex = Protocol::HeaderOrder;

size_t get_buffered_rx_length();

namespace UartFunctions {

ParseResult validate_frame(const Protocol::Frame &buffer);
uint16_t crc16_modbus(const uint8_t *data, size_t length);
constexpr uint16_t merge_uint8(uint8_t first_bit, uint8_t second_bit) {
  return (static_cast<uint16_t>(first_bit)) |
         static_cast<uint16_t>(second_bit << 8);
}

std::optional<IndexedFrame> create_indexed_frame(const SizedReadBuffer &buffer,
                                                 size_t start_index);
UartMessage reconstruct_uart_message(const Protocol::Frame &buffer);
} // namespace UartFunctions
