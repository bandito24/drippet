#include "self_node.hpp"
#include "driver.hpp"
#include "protocol.hpp"
#include <cstdint>
#include <optional>

std::optional<UartMessage> SelfNode::handle_incoming_frame(UartMessage &msg) {
  return std::nullopt;
}
UartMessage SelfNode::generate_discovery_message() {
  assert(this->self_addr == ADDR_UNSET);
  return {.address = ADDR_UNSET,
          .command = Protocol::Command::DISCOVERY,
          .data = Protocol::FrameDataArray{this->self_key},
          .data_length = 1};
}
void SelfNode::initialize_watering(UartMessage &msg) {}
