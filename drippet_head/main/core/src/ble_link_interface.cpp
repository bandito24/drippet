#include "ble_link_interface.hpp"
#include "ble_types.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "external_requests.hpp"
#include "logger.hpp"
#include "type_cast.hpp"
#include "util.hpp"
#include <algorithm>

// FOR WRITES
void BLELinkInterface::handle_writes(std::span<const uint8_t> raw_data) {

  BLE::Cmds cmd = static_cast<BLE::Cmds>(raw_data[CMD_IDX]);
  switch (cmd) {
  case BLE::Cmds::INIT_PAIRING:
    this->head_node.ext_req_pairing_mode();
    break;
  case BLE::Cmds::WRITE_CONF_TIME:
    this->head_node.ext_req_set_clock(raw_data[BLE::HOUR_IDX],
                                      raw_data[BLE::MINUTE_IDX]);
    break;

  case BLE::Cmds::WRITE_CONF_PHASE:
    this->head_node.ext_req_set_phase(raw_data[BLE::DATA_START_IDX]);
    break;
  case BLE::Cmds::WRITE_CONF_TIME_PHASE:
    this->head_node.ext_req_set_phase_time(raw_data[BLE::HOUR_IDX],
                                           raw_data[BLE::MINUTE_IDX]);
    break;

  case BLE::Cmds::WRITE_NODE_DURATION: {

    size_t target_row = raw_data[BLE::TGT_ROW_IDX];
    uint16_t new_duration = Util::get_le16(&raw_data[BLE::TGT_ROW_DATA_IDX]);
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
    std::copy(nodes_state.begin(), nodes_state.end(),
              this->buffer.data.begin());
    this->buffer.len = nodes_state.size();

    break;
  }
  case BLE::Read_T::READ_CONFIGURATION: {
    // Probably need a flag whether next phase is set. Also what the current
    // phase of cycle is
    // Read is as follows:
    // 1) curr_hour and min for index 1 & 2,
    // 2) phase of cycle for index 3
    // 3) index 4: (boolean flag) 1 for phase time set, 0 for phase time not set
    // 4) index 5-6: hour and min of next phase (0,0 for not set)

    HourMin curr_time = this->head_node.get_hourmin_curr_time();
    std::optional<HourMin> next_phase =
        this->head_node.get_hourmin_next_phase();
    CyclePhase phase_of_cycle = this->head_node.get_phase_pf_cycle();
    uint8_t phase_set = next_phase ? 1 : 0;

    std::array<uint8_t, 7> cfg{curr_time.hour, curr_time.minute,
                               static_cast<uint8_t>(phase_of_cycle), phase_set};

    cfg[5] = next_phase ? next_phase->hour : 0;
    cfg[6] = next_phase ? next_phase->minute : 0;
    std::copy(cfg.begin(), cfg.end(), this->buffer.data.begin());
    this->buffer.len = cfg.size();
  }

  case BLE::Read_T::READ_EVENTS: {

    OptionalRequest rsp = this->head_node.extRequestsManager.popEvent();
    if (!rsp) {
      Logger::log_error("No read available on external response");
      return this->buffer;
    }
    this->buffer.data[0] = static_cast<uint8_t>(rsp->type);
    std::copy(rsp->data.begin(), rsp->data.end(),
              this->buffer.data.begin() + 1);
    this->buffer.len = rsp->data.size() + 1;
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
  }
  default:
    Logger::log_error("Unrecognized type in handle_write_buffer of %d", type);
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
