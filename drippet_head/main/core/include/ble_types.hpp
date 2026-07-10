#pragma once
#include <string>
namespace BLE {

// Header
//[CMD, LEN]

// LOAD_ROW:
//[CMD, LEN, ROW] //data length 1

// WRITE_NODE_DURATION:
//[CMD, LEN, ROW, VALUE_LO, VALUE_HI] // data length 3

// WRITE_NODE_CYCLE:
//[CMD, LEN, ROW, uint8_t BITMASK] // data length 2
constexpr std::string DEVICE_NAME = "Drippet";
enum class Cmds {
  WRITE_CONF_TIME,
  WRITE_NODE_DURATION,
  WRITE_NODE_CYCLE,
  WRITE_CONF_PHASE,
  WRITE_CONF_TIME_PHASE,
  // NOTE: Init_pairing should be the last thing since node durations will be
  // reset with init pairing
  INIT_PAIRING,
  REQUEST_COUNT
};
enum class Read_T {
  READ_NODE_STATES,
  READ_NODE_DURATIONS,
  READ_CONFIGURATION, // Time, Phase, and Head state(?)
  READ_EVENTS

};

constexpr size_t HEADER_LEN = 1;
enum CONF_DATA_IDX { HOUR = BLE::HEADER_LEN, MINUTE = BLE::HEADER_LEN + 1 };
const size_t DATA_START_IDX = HEADER_LEN;
const size_t HOUR_IDX = static_cast<size_t>(CONF_DATA_IDX::HOUR);
const size_t MINUTE_IDX = static_cast<size_t>(CONF_DATA_IDX::MINUTE);
enum class Header : size_t { COMMAND };
constexpr uint8_t MAX_INCOMING_PKT_LEN = 20;

constexpr size_t TGT_ROW_IDX = HEADER_LEN + 1;
constexpr size_t TGT_ROW_DATA_IDX = TGT_ROW_IDX + 1;
constexpr size_t WRITE_CEL_DATA_LEN = 3;   // Row and two uint16_t fragments
constexpr size_t WRITE_NODE_CYCLE_DATA_LEN = 2; // Row and bitmask
constexpr size_t FLAT_BITMASK_IDX = 2;
// Row, 2 uint16_t fragments, bitmask for cycle
constexpr size_t DURATION_BUFF_LEN = 1 + 2 + 1;
constexpr size_t MTU_SIZE = 110;

constexpr size_t MAX_PKT_LEN = HEADER_LEN + DURATION_BUFF_LEN;

enum class Status {
  OP_OK,
  INVALID_NODE,
  INVALID_HOSE,
  INVALID_CMD,
  INVALID_PKT_LEN
};
} // namespace BLE
