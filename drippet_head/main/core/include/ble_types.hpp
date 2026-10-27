#pragma once
#include <string>
namespace BLE {

constexpr std::string DEVICE_NAME = "Drippet";
enum class Cmds {
  // NOTE: WRITE_CONF_TIME will now handle time and phase with timestamp
  WRITE_CONF_TIME,
  WRITE_NODE_DURATION,
  WRITE_NODE_CYCLE,
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
const size_t CONF_TIME_TS_IDX = HEADER_LEN;
const size_t CONF_TIME_OFFSET_IDX = HEADER_LEN + 8;  // (9)
const size_t CONF_TIME_PKT_LEN = HEADER_LEN + 8 + 2; // (11)
// READ_CONFIGURATION response (no header byte)
constexpr size_t READ_CONF_TS_IDX = 0;               // int64_t UTC seconds
constexpr size_t READ_CONF_OFFSET_IDX = 8;           // int16_t offset minutes
constexpr size_t READ_CONF_WATER_SET_IDX = 10;       // 0/1
constexpr size_t READ_CONF_WATER_HOUR_IDX = 11;      // local 0-23
constexpr size_t READ_CONF_WATER_MIN_IDX = 12;       // local 0-59
constexpr size_t READ_CONF_PKT_LEN = 13;
const size_t DATA_START_IDX = HEADER_LEN;
enum class Header : size_t { COMMAND };
constexpr uint8_t MAX_INCOMING_PKT_LEN = 20;

const size_t HOUR_IDX = static_cast<size_t>(CONF_DATA_IDX::HOUR);
const size_t MINUTE_IDX = static_cast<size_t>(CONF_DATA_IDX::MINUTE);

constexpr size_t TGT_ROW_IDX = HEADER_LEN;
constexpr size_t TGT_ROW_DATA_IDX = TGT_ROW_IDX + 1;
constexpr size_t WRITE_CEL_DATA_LEN = 3; // Row and two uint16_t fragments
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
