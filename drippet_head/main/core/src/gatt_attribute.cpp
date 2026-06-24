#include "gatt_attribute.hpp"
#include "clock.hpp"
#include "config.hpp"
#include "head.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "protocol_types.hpp"
#include "util.hpp"
#include <cassert>
#include <optional>

FlatSchedule GattAttribute::duration_schedule_to_bytes(
    const NodeTypes::DurationSchedule &schedule, config::Address address) {
  FlatSchedule res{};
  res[0] = address;
  Util::put_le16(res.data() + 1, schedule.duration);
  uint8_t bitmask = Util::water_cycle_to_bytes(schedule.cycle);
  res[3] = bitmask;
  return res;
}

NodeTypes::DurationSchedule
GattAttribute::bytes_to_duration_schedule(const FlatSchedule &byte_sch) {
  auto schedule = NodeTypes::DurationSchedule{};
  schedule.duration = Util::get_le16(byte_sch.data() + 1);
  uint8_t bitmask = byte_sch.at(BLE::FLAT_BITMASK_IDX);

  schedule.cycle = Util::byte_to_water_cycle(bitmask);
  return schedule;
}

BLE::Status GattAttribute::load_duration_buffer(size_t row) {
  if (row >= config::max_nodes) {
    return BLE::Status::INVALID_NODE;
  }
  std::optional<NodeTypes::DurationSchedule> duration =
      this->head.get_node_duration_schedule(row);
  if (!duration) {
    return BLE::Status::INVALID_NODE;
  }
  this->duration_buffer =
      GattAttribute::duration_schedule_to_bytes(*duration, row);

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
    uint16_t new_duration = Util::get_le16(&raw_data[BLE::DATA_START_IDX]);

    this->head.ext_req_set_node_duration(new_duration, target_row);
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
    expected_len = BLE::WRITE_CEL_DATA_LEN;
    break;
  case BLE::Cmds::WRITE_CYCLE:
    expected_len = BLE::WRITE_CYCLE_DATA_LEN;
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
