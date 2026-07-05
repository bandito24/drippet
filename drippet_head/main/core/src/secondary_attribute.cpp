#include "secondary_attribute.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "logger.hpp"
#include <cstdint>

void SysConfigAttr::handle_ext_write_conf(const std::span<uint8_t> &raw_data) {
  BLE::Cmds cmd = static_cast<BLE::Cmds>(raw_data[0]);
  uint8_t hour = raw_data[HOUR_IDX];
  uint8_t min = raw_data[MIN_IDX];
  switch (cmd) {
  case BLE::Cmds::WRITE_CONF_TIME:
    this->head_node.ext_req_set_clock(hour, min);
    break;
  case BLE::Cmds::WRITE_CONF_PHASE:
    this->head_node.ext_req_set_phase(hour, min);
    break;
  case BLE::Cmds::INIT_PAIRING:
    this->head_node.ext_req_pairing_mode();
    break;

  default:
    Logger::log_error("Unrecognized conf write command of %d", raw_data[0]);
  }
}
