#include "ble_link_interface.hpp"
#include "ble_types.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "external_requests.hpp"
#include "logger.hpp"
#include "util.hpp"
#include <algorithm>
#include <chrono>

// FOR WRITES
void BLELinkInterface::handle_writes(std::span<const uint8_t> raw_data) {

  BLE::Cmds cmd = static_cast<BLE::Cmds>(raw_data[CMD_IDX]);
  switch (cmd) {
  case BLE::Cmds::INIT_PAIRING:
    this->head_node.ext_req_pairing_mode();
    break;
  case BLE::Cmds::WRITE_CONF_TIME: {

    // A short packet is zero-padded, so ts = 0 and set_time rejects it
    auto ts = static_cast<Time::Epoch>(
        Util::get_le64(&raw_data[BLE::CONF_TIME_TS_IDX]));
    auto offset_min = static_cast<Time::UtcOffsetMin>(
        Util::get_le16(&raw_data[BLE::CONF_TIME_OFFSET_IDX]));
    this->head_node.ext_req_set_clock(ts, offset_min);

    break;
  }

  case BLE::Cmds::WRITE_CONF_TIME_PHASE:
    this->head_node.ext_req_set_phase_time(raw_data[BLE::HOUR_IDX],
                                           raw_data[BLE::MINUTE_IDX]);
    break;

  case BLE::Cmds::WRITE_NODE_DURATION: {

    size_t target_row = raw_data[BLE::TGT_ROW_IDX];
    uint16_t new_duration = Util::get_le16(&raw_data[BLE::TGT_ROW_DATA_IDX]);

    Logger::log_simple("DOING IT for %d, with %d", target_row, new_duration);
    this->head_node.ext_req_set_node_duration(target_row, new_duration);
    break;
  }
  case BLE::Cmds::WRITE_NODE_CYCLE:
    this->head_node.ext_req_set_node_cycle(raw_data[BLE::TGT_ROW_IDX],
                                           raw_data[BLE::TGT_ROW_DATA_IDX]);
    break;

  default:
    Logger::log_error("Unrecognized command in handle_read_buffer of %d", cmd);
  }
  // Clear buffer after read
  this->reset_buffer(this->buffer);
}
// FOR READS
const SerializedPacketBuffer &BLELinkInterface::handle_reads(BLE::Read_T type) {
  // Clear buffer before write
  this->reset_buffer(this->buffer);

  switch (type) {
  case BLE::Read_T::READ_NODE_STATES: {
    all_node_status_t nodes_state = this->head_node.get_node_statuses();
    size_t node_count = this->head_node.get_discovered_node_count();
    std::copy(nodes_state.begin(), nodes_state.begin() + node_count,
              this->buffer.data.begin());
    this->buffer.len = node_count;

    break;
  }
  case BLE::Read_T::READ_CONFIGURATION: {
    // Layout documented above handle_reads in ble_link_interface.hpp

    Time::Time_Point curr_time = this->head_node.get_curr_utc_time();
    std::optional<HourMin> start = this->head_node.get_hourmin_next_phase();
    auto &out = this->buffer.data;

    Util::put_le64(&out[BLE::READ_CONF_TS_IDX],
                   std::chrono::system_clock::to_time_t(curr_time));
    Util::put_le16(&out[BLE::READ_CONF_OFFSET_IDX],
                   static_cast<uint16_t>(this->head_node.get_utc_offset_min()));
    out[BLE::READ_CONF_WATER_SET_IDX] = start ? 1 : 0;
    out[BLE::READ_CONF_WATER_HOUR_IDX] = start ? start->hour : 0;
    out[BLE::READ_CONF_WATER_MIN_IDX] = start ? start->minute : 0;
    this->buffer.len = BLE::READ_CONF_PKT_LEN;
    break;
  }

  case BLE::Read_T::READ_EVENTS: {

    Logger::log_simple("ATTEMPTING TO SEND OUT EVENT");
    OptionalRequest rsp = this->head_node.extRequestsManager.popEvent();
    if (!rsp) {
      Logger::log_error("No read available on external response");
      return this->buffer;
    }
    this->buffer.data[0] = static_cast<uint8_t>(rsp->type);
    std::copy(rsp->data.begin(), rsp->data.end(),
              this->buffer.data.begin() + 1);
    this->buffer.len = rsp->data.size() + 1;
    break;
  }
  case BLE::Read_T::READ_NODE_DURATIONS: {
    std::optional<NodeTypes::DurationSchedule> curr{};
    size_t node_count = 0, insert_idx = 0;
    while (node_count < config::max_nodes) {

      curr = this->head_node.get_node_duration_schedule(node_count);
      if (curr) {
        FlatNodeSchedule sch =
            BLELinkInterface::duration_schedule_to_bytes(*curr);
        std::copy(sch.begin(), sch.end(),
                  this->buffer.data.begin() + insert_idx);

        node_count++;
        insert_idx += sch.size();
      } else {
        break;
      }
    }
    this->buffer.len = insert_idx;
    break;
  }
  default:
    Logger::log_error("Unrecognized type in handle read buffer of %d", type);
  }
  return this->buffer;
}

FlatNodeSchedule BLELinkInterface::duration_schedule_to_bytes(
    const NodeTypes::DurationSchedule &schedule) {
  FlatNodeSchedule res{};
  Util::put_le16(res.data(), schedule.duration);
  uint8_t bitmask = Util::water_cycle_to_bytes(schedule.cycle);
  res[2] = bitmask;
  return res;
}

NodeTypes::DurationSchedule
BLELinkInterface::bytes_to_duration_schedule(const FlatNodeSchedule &byte_sch) {
  auto schedule = NodeTypes::DurationSchedule{};
  schedule.duration = Util::get_le16(byte_sch.data());
  uint8_t bitmask = byte_sch.at(BLE::FLAT_BITMASK_IDX);

  schedule.cycle = Util::byte_to_water_cycle(bitmask);
  return schedule;
}
