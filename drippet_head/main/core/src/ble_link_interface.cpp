#include "ble_link_interface.hpp"
#include "ble_types.hpp"
#include "logger.hpp"
#include "secondary_attribute.hpp"
#include "type_cast.hpp"
#include "util.hpp"

void BLELinkInterface::handle_read_buffer() {
  if (this->incoming_buffer.len == 0) {
    Logger::log_error("Requesting Read on empty buffer");
    return;
  }

  const DataPkt &raw_data = this->incoming_buffer.data;
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
    this->head_node.ext_req_set_phase(raw_data[BLE::HOUR_IDX],
                                      raw_data[BLE::MINUTE_IDX]);
    break;

  case BLE::Cmds::WRITE_CELL: {

    size_t target_row = raw_data[BLE::TGT_ROW_IDX];
    uint16_t new_duration = Util::get_le16(&raw_data[BLE::TGT_ROW_DATA_IDX]);
    this->head_node.ext_req_set_node_duration(target_row, new_duration);
    break;
  }
  case BLE::Cmds::WRITE_CYCLE:
    this->head_node.ext_req_set_node_cycle(raw_data[BLE::TGT_ROW_IDX],
                                           raw_data[BLE::TGT_ROW_DATA_IDX]);
    break;

  default:
    Logger::log_error("Unrecognized command in handle_read_buffer of %d", cmd);
  }
  this->reset_buffer(this->incoming_buffer);
}
