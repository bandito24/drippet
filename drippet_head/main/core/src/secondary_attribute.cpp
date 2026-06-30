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
    this->request_head_write_clock(hour, min);
    break;
  case BLE::Cmds::WRITE_CONF_PHASE:
    this->request_head_write_phase_start(hour, min);
    break;
  default:
    Logger::log_error("Unrecognized conf write command of %d", raw_data[0]);
  }
}
void SysConfigAttr::request_head_write_clock(uint8_t hour, uint8_t min) {
  this->head_node.ext_req_set_clock(hour, min);
}
void SysConfigAttr::request_head_write_phase_start(uint8_t hour, uint8_t min) {
  this->head_node.ext_req_set_phase(hour, min);
}
OptionalRequest ExtReqResponseAttr::get_request_response() {
  OptionalRequest rsp = this->head_node.extRequestsManager.popEvent();
  return rsp;
}
