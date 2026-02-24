#pragma once
#include "config.hpp"
#include <array>
#include <cstddef>
#include <optional>
#include <stdint.h>

using Pin = uint8_t;
using Esp_Err_t = uint8_t;
using byte_count = int;

namespace Protocol {

constexpr uint8_t start_bit = 0xAA;
constexpr uint8_t no_address = 200;
constexpr uint8_t head_address = 0;

enum class Command : uint8_t {
  DISCOVERY,
  ADDRESSING,
  BROADCAST,
  INIT_WATER_DURATIONS
};

enum class HeaderOrder : uint8_t {
  START_BIT,
  ADDRESS,
  COMMAND,
  DATA_LENGTH,
  HEADER_LENGTH
}; // Header length strictly for reference and not included
constexpr size_t DATA_START_INDEX =
    static_cast<size_t>(Protocol::HeaderOrder::HEADER_LENGTH);

constexpr size_t FrameLength =
    static_cast<size_t>(Protocol::HeaderOrder::HEADER_LENGTH) +
    (config::node_hose_count * 2) + 2;

using Frame = std::array<uint8_t, FrameLength>;

using FrameDataArray = std::array<uint16_t, config::node_hose_count>;
} // namespace Protocol
//
//
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

struct UartMessage {
  uint8_t address{};
  Protocol::Command command{};
  Protocol::FrameDataArray data{}; // Represents the max data size
  size_t data_length{};
};

struct Uart {
  virtual Esp_Err_t init() = 0;
  virtual byte_count write_bytes(UartMessage) const = 0;
};
inline std::optional<size_t>
calculate_last_index(std::span<const uint8_t> buffer, size_t start_index) {
  assert(buffer[start_index] == Protocol::start_bit);

  size_t data_length_index =
      start_index + to_index(Protocol::HeaderOrder::DATA_LENGTH);
  size_t last_index = start_index +
                      to_index(Protocol::HeaderOrder::HEADER_LENGTH) +
                      (buffer[data_length_index]) + 2 - 1; // 2 For CRC16
  if (last_index >= buffer.size()) {
    return std::nullopt;
  }
  return last_index;
}

class EspUart : public Uart {
public:
  Esp_Err_t init() override;

  byte_count write_bytes(UartMessage) const override;
  Esp_Err_t broadcast() const;
  EspUart(Pin tx, Pin rx);
  EspUart();

private:
  Pin tx_pin;
  Pin rx_pin;
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

namespace UartFunctions {

ParseResult validate_frame(Protocol::Frame buffer);
uint16_t crc16_modbus(const uint8_t *data, size_t length);
uint16_t merge_uint8(uint8_t high_bit, uint8_t low_bit);

UartMessage reconstruct_uart_message(Protocol::Frame buffer);
} // namespace UartFunctions
