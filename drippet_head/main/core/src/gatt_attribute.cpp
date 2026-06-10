#include "gatt_attribute.hpp"
#include "clock.hpp"
#include "config.hpp"
#include "head.hpp"
#include "node.hpp"
#include "protocol_types.hpp"
#include "util.hpp"
#include <cassert>
#include <optional>

BLE::Status GattAttribute::load_duration_buffer(size_t row) {
  if (row >= config::max_nodes) {
    return BLE::Status::INVALID_NODE;
  }
  std::optional<NodeTypes::HoseDuration> duration =
      this->head.get_node_hose_duration(row);
  if (!duration) {
    return BLE::Status::INVALID_NODE;
  }

  uint8_t buffer[Protocol::MAX_DATA_LEN * 2]{};
  Util::put_le16(buffer, duration.value());
  // TODO: add phase cycle
  std::copy(std::begin(buffer), std::end(buffer),
            this->duration_buffer.begin() + 1);
  this->duration_buffer[0] = row;

  return BLE::Status::OP_OK;
}

BLE::Status GattAttribute::handle_incoming_write(std::span<uint8_t> raw_data) {
  BLE::Cmds CMD = static_cast<BLE::Cmds>(raw_data[0]);
  size_t target_row = raw_data[BLE::TGT_ROW_IDX];

  BLE::Status status = this->validate_packet(raw_data);
  if (status != BLE::Status::OP_OK) {
    Logger::log_error("Invalid packet status of %d with operation %d", status,
                      CMD);
    return status;
  }

  switch (CMD) {
  case BLE::Cmds::LOAD_ROW: {
    return this->load_duration_buffer(target_row);
    break;
  }
  case BLE::Cmds::WRITE_CELL: {
    // FIX: THE ADDRESSING, wont be accurate
    uint16_t new_duration = Util::get_le16(&raw_data[BLE::TGT_ROW_IDX + 2]);

    this->head.set_node_duration(target_row, new_duration);
    return this->load_duration_buffer(target_row);
    break;
  }
  case BLE::Cmds::WRITE_ROW: {

    // FIX: Do we want this? idk, fix it
    std::optional<NodeTypes::HoseDuration> hose_durations =
        this->head.get_node_hose_duration(target_row);
    return this->load_duration_buffer(target_row);
    break;
  }
  default:
    return BLE::Status::INVALID_CMD;
  };
}

BLE::Status GattAttribute::validate_packet(std::span<uint8_t> pkt) {
  if (pkt[BLE::TGT_ROW_IDX] >= config::max_nodes) {
    return BLE::Status::INVALID_NODE;
  }
  if (!this->head.node_exists(pkt[BLE::TGT_ROW_IDX])) {
    return BLE::Status::INVALID_NODE;
  }

  BLE::Cmds CMD = static_cast<BLE::Cmds>(pkt[0]);
  size_t expected_len;
  switch (CMD) {
  case BLE::Cmds::LOAD_ROW:
    expected_len = BLE::LOAD_ROW_DATA_LEN;
    break;
  case BLE::Cmds::WRITE_CELL:
    if (pkt[BLE::TGT_CELL_IDX] >= Protocol::MAX_DATA_LEN) {
      return BLE::Status::INVALID_HOSE;
    }
    expected_len = BLE::WRITE_CEL_DATA_LEN;
    break;
  case BLE::Cmds::WRITE_ROW:
    expected_len = BLE::WRITE_ROW_DATA_LEN;
    break;
  default:
    return BLE::Status::INVALID_CMD;
  }
  if (pkt.size() <= BLE::HEADER_LEN || pkt[BLE::DATA_LEN_IDX] != expected_len) {
    Logger::log_error("received packet length of %d, expected %d",
                      pkt[BLE::DATA_LEN_IDX], expected_len);
    return BLE::Status::INVALID_PKT_LEN;
  }
  return BLE::Status::OP_OK;
}
