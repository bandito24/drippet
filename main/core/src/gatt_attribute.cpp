#include "gatt_attribute.hpp"
#include "logger.hpp"

void GattAttribute::load_duration_buffer(size_t row) {
  assert(row < config::max_nodes);
  size_t insert_addr = 0;
  for (size_t i = 0; i < config::node_hose_count; i++) {

    put_le16(&this->duration_buffer[insert_addr], this->durations[row][i]);
    insert_addr += 2;
  }
}

void GattAttribute::handle_incoming_write(std::span<uint8_t> raw_data) {
  BLE::Cmds CMD = static_cast<BLE::Cmds>(raw_data[0]);
  size_t target_row = raw_data[BLE::TGT_ROW_IDX];
  if (target_row >= config::max_nodes) {
    Logger::log_error("Invalid row passed in of %d", target_row);
  }

  switch (CMD) {
  case BLE::Cmds::LOAD_ROW: {
    this->load_duration_buffer(target_row);
    break;
  }
  case BLE::Cmds::WRITE_CELL: {
    size_t target_col = raw_data[BLE::TGT_ROW_IDX + 1];
    uint16_t new_duration = get_le16(&raw_data[BLE::TGT_ROW_IDX + 2]);
    set_node_duration(target_row, target_col, new_duration);
    this->load_duration_buffer(target_row);
    break;
  }
  case BLE::Cmds::WRITE_ROW: {
    size_t start_idx = BLE::TGT_ROW_IDX + 1;
    for (size_t target_col = 0; target_col < config::node_hose_count;
         target_col++) {
      uint16_t new_duration = get_le16(&raw_data[start_idx]);
      set_node_duration(target_row, target_col, new_duration);
      start_idx += 2;
    }
    this->load_duration_buffer(target_row);
    break;
  }
  default:
    return;
  };
}

void GattAttribute::set_node_duration(size_t node, size_t hose,
                                      uint16_t duration) {
  assert(node <= config::max_nodes);
  assert(hose <= config::node_hose_count);
  this->durations[node][hose] = duration;
}
