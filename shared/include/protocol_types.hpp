
#include "config.hpp"
#include <cstddef>
#include <stdint.h>
namespace Protocol {

constexpr uint8_t start_bit = 0xAA;
constexpr uint8_t head_address = 0x00;

enum class Command : uint8_t {
  DISCOVERY,
  ADDRESSING,
  BROADCAST,
  INIT_WATER_DURATIONS,
  ACK,
  STATUS,
  BUGGER_OFF
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

using LocalReadBuffer = std::array<uint8_t, 128>;
struct SizedReadBuffer {
  LocalReadBuffer content;
  size_t length;
};
