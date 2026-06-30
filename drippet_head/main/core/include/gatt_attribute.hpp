#pragma once
#include "config.hpp"
#include "constants.hpp"
#include "head.hpp"
#include "node.hpp"
#include "protocol_types.hpp"
#include <span>
namespace BLE {

// Header
//[CMD, LEN]

// LOAD_ROW:
//[CMD, LEN, ROW] //data length 1

// WRITE_CELL:
//[CMD, LEN, ROW, VALUE_LO, VALUE_HI] // data length 3

// WRITE_CYCLE:
//[CMD, LEN, ROW, uint8_t BITMASK] // data length 2
constexpr std::string DEVICE_NAME = "Drippet";
enum class Cmds {
  LOAD_ROW,
  WRITE_CYCLE,
  WRITE_CELL,
  WRITE_CONF_TIME,
  WRITE_CONF_PHASE
};
enum class Header : size_t { COMMAND, DATA_LEN };

constexpr size_t DATA_LEN_IDX = static_cast<size_t>(Header::DATA_LEN);
constexpr size_t TGT_ROW_IDX = DATA_LEN_IDX + 1;
constexpr size_t DATA_START_IDX = TGT_ROW_IDX + 1;
constexpr size_t HEADER_LEN = 2;
constexpr size_t LOAD_ROW_DATA_LEN = 1;    // Row
constexpr size_t WRITE_CEL_DATA_LEN = 3;   // Row and two uint16_t fragments
constexpr size_t WRITE_CYCLE_DATA_LEN = 2; // Row and bitmask
//
// Row, 2 uint16_t fragments, bitmask for cycle
constexpr size_t DURATION_BUFF_LEN = 1 + 2 + 1;
// constexpr size_t MAX_PKT_LEN = HEADER_LEN + DURATION_BUFF_LEN;
//
constexpr size_t MAX_PKT_LEN = 20;
constexpr size_t FLAT_DURATION_IDX = 1;
constexpr size_t FLAT_BITMASK_IDX = 3;

enum class Status {
  OP_OK,
  INVALID_NODE,
  INVALID_HOSE,
  INVALID_CMD,
  INVALID_PKT_LEN
};
} // namespace BLE

using FlatSchedule = std::array<uint8_t, 4>;

class GattAttribute {
private:
  Head &head;
  BLE::Status validate_packet(std::span<uint8_t> pkt);
  // Two duration bytes for uint16_t then a bitmask for cycle

public:
  static FlatSchedule
  duration_schedule_to_bytes(const NodeTypes::DurationSchedule &dur_sch,
                             config::Address address);

  static NodeTypes::DurationSchedule
  bytes_to_duration_schedule(const FlatSchedule &byte_sch);
  GattAttribute(Head &headNode) : head{headNode} {};
  BLE::Status load_duration_buffer(size_t addr);
  FlatSchedule duration_buffer{};
  // An addr specifier, 2 uint16_t to uint8_t, and a bitmask for water cycle

  BLE::Status handle_incoming_write(std::span<uint8_t> raw_data);
  uint16_t duration_chr_handle;
};
