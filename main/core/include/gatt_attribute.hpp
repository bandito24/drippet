#pragma once
#include "config.hpp"
#include "head.hpp"
#include <span>
namespace BLE {

// Header
//[CMD, LEN]

// LOAD_ROW:
//[CMD, LEN, ROW] //data length 1

// WRITE_ROW:
//[CMD, LEN, ROW, DATA...] //data length 11

// WRITE_CELL:
//[CMD, LEN, ROW, COL, VALUE_LO, VALUE_HI] // data length 4

constexpr std::string DEVICE_NAME = "Drippet";
enum class Cmds { LOAD_ROW, WRITE_ROW, WRITE_CELL };
enum class Header : size_t { COMMAND, DATA_LEN };
constexpr size_t TGT_ROW_IDX = 2;
constexpr size_t TGT_CELL_IDX = TGT_ROW_IDX + 1;
constexpr size_t DATA_LEN_IDX = static_cast<size_t>(Header::DATA_LEN);

constexpr size_t HEADER_LEN = 2;
constexpr size_t LOAD_ROW_DATA_LEN = 1; // Header plus 1 row
constexpr size_t WRITE_ROW_DATA_LEN =
    (config::node_hose_count * 2) + 1;   // uint16_t fragments plus header
constexpr size_t WRITE_CEL_DATA_LEN = 4; // Row, Column, and uint16_t fragments
constexpr size_t MAX_PKT_LEN = HEADER_LEN + WRITE_ROW_DATA_LEN;
constexpr size_t DURATION_BUFF_LEN = 1 + (2 * config::node_hose_count);

enum class Status {
  OP_OK,
  INVALID_NODE,
  INVALID_HOSE,
  INVALID_CMD,
  INVALID_PKT_LEN
};
} // namespace BLE

class GattAttribute {
private:
  Head &head;
  BLE::Status validate_packet(std::span<uint8_t> pkt);

public:
  GattAttribute(Head &headNode) : head{headNode} {};
  BLE::Status load_duration_buffer(size_t addr);
  std::array<uint8_t, config::node_hose_count * 2 + 1> duration_buffer{};
  // 2 uint8_t bytes for each duration with a first addr specifier

  BLE::Status handle_incoming_write(std::span<uint8_t> raw_data);
  BLE::Status set_node_duration(size_t node, size_t hose, uint16_t duration);
};
