#include "gatt_attribute.hpp"
#include "clock.hpp"
#include "config.hpp"
#include "node.hpp"
#include "util.hpp"
#include <cassert>

BLE::Status GattAttribute::load_duration_buffer(size_t row) {
  if (row >= config::max_nodes) {
    return BLE::Status::INVALID_NODE;
  }
  iNode *node = this->head.get_node(row);
  if (node == nullptr) {
    return BLE::Status::INVALID_NODE;
  }
  size_t insert_addr = 0;
  NodeTypes::HoseDurations durations = node->get_all_hose_durations();

  uint8_t buffer[config::node_hose_count * 2]{};
  Util::le16_to_le8(buffer, durations);
  std::copy(std::begin(buffer), std::end(buffer),
            this->duration_buffer.begin() + 1);
  this->duration_buffer[0] = row;
  return BLE::Status::OP_OK;
}

BLE::Status GattAttribute::handle_incoming_write(std::span<uint8_t> raw_data) {
  BLE::Cmds CMD = static_cast<BLE::Cmds>(raw_data[0]);
  size_t target_row = raw_data[BLE::TGT_ROW_IDX];
  if (target_row >= config::max_nodes) {
    return BLE::Status::INVALID_NODE;
  }

  switch (CMD) {
  case BLE::Cmds::LOAD_ROW: {
    return this->load_duration_buffer(target_row);
    break;
  }
  case BLE::Cmds::WRITE_CELL: {
    size_t target_col = raw_data[BLE::TGT_ROW_IDX + 1];
    if (target_col >= config::node_hose_count) {
      return BLE::Status::INVALID_HOSE;
    }
    uint16_t new_duration = Util::get_le16(&raw_data[BLE::TGT_ROW_IDX + 2]);
    set_node_duration(target_row, target_col, new_duration);
    return this->load_duration_buffer(target_row);
    break;
  }
  case BLE::Cmds::WRITE_ROW: {
    size_t start_idx = BLE::TGT_ROW_IDX + 1;
    for (size_t target_col = 0; target_col < config::node_hose_count;
         target_col++) {
      uint16_t new_duration = Util::get_le16(&raw_data[start_idx]);
      set_node_duration(target_row, target_col, new_duration);
      start_idx += 2;
    }
    return this->load_duration_buffer(target_row);
    break;
  }
  default:
    return BLE::Status::INVALID_CMD;
  };
}

BLE::Status GattAttribute::set_node_duration(size_t addr, size_t hose,
                                             uint16_t duration) {
  assert(addr <= config::max_nodes);
  assert(hose <= config::node_hose_count);
  iNode *node = this->head.get_node(addr);
  if (node == nullptr) {
    return BLE::Status::INVALID_NODE;
  }
  node->edit_hose_duration(hose, static_cast<Time::Time_Seconds>(duration));
  return BLE::Status::OP_OK;
}
