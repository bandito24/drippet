#pragma once
#include "config.hpp"
namespace BLE {

constexpr std::string DEVICE_NAME = "Drippet";
enum class Cmds { LOAD_ROW, WRITE_ROW, WRITE_CELL };
enum class Header : size_t { COMMAND, DATA_LEN };
constexpr size_t TGT_ROW_IDX = 3;
constexpr size_t HEADER_LEN = 2;
constexpr size_t LOAD_ROW_LEN = HEADER_LEN + 1; // Header plus 1 row
constexpr size_t WRITE_ROW_LEN =
    HEADER_LEN + (config::node_hose_count * 2); // uint16_t fragments
constexpr size_t WRITE_CEL_LEN =
    HEADER_LEN + 4; // Row, Column, and uint16_t fragments
constexpr size_t MAX_LEN = WRITE_ROW_LEN;
constexpr size_t DURATION_BUFF_LEN = config::node_hose_count * 2;
} // namespace BLE

class GattAttribute {
private:
public:
  void load_duration_buffer(size_t row);
  uint16_t durations[config::max_nodes][config::node_hose_count] = {
      {1000, 2000, 3000, 4000, 5000}, {6000, 7000, 8000, 9000, 10000}};
  uint8_t duration_buffer[BLE::DURATION_BUFF_LEN];

  void handle_incoming_write(std::span<uint8_t> raw_data);
  void handle_write_cell(std::span<uint8_t> raw_data);
  void handle_write_row(std::span<uint8_t> raw_data);

  void handle_load_row(std::span<uint8_t> raw_data);
  void set_node_duration(size_t node, size_t hose, uint16_t duration);
};
