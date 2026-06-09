#include "self_node.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "logger.hpp"
#include "protocol.hpp"
#include "util.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

using CMD = Protocol::Command;
std::optional<UartMessage> SelfNode::handle_incoming_frame(UartMessage &msg) {

  this->update_last_evt_time();
  if (msg.command == CMD::DISCOVERY || msg.command == CMD::ADDRESSING) {
    // These are the only possible messages one can receive without addressing.
    // Key is used for endpoint
    return this->handle_unsynced_message(msg);
  }
  if (!this->has_addr() || msg.address != this->self_addr) {
    return std::nullopt;
  }
  // At this point we know its a certified command;
  switch (msg.command) {
  case CMD::ACK: // Only received after sending back key and address and being
                 // recognizaed by head
    this->complete_initialization();
    break;
  case CMD::INIT_WATER_DURATIONS:
    return this->initialize_watering(msg.data);
  case CMD::STATUS:
    return UartMessage{.address = self_addr,
                       .command = CMD::STATUS,
                       .data = {static_cast<uint8_t>(this->status)},
                       .data_length = 1};
  default: {
    Logger::log_error("Invalid command of %d", msg.command);
    return std::nullopt;
  }
  }
  return std::nullopt;
}

std::optional<UartMessage> SelfNode::handle_unsynced_message(UartMessage &msg) {
  if (this->status != NodeStatus::INITIALIZING) {
    // Only respond to unsynced messages when not initialized and using the key
    // head sends
    return std::nullopt;
  }
  NodeKey_t received_key = Util::deserialize_key(msg.data);
  if (msg.command == CMD::DISCOVERY) {
    if (!this->self_key) {
      this->self_key = received_key;
      this->serial_key = Util::serialize_key(this->self_key);
    }
    return UartMessage{.address = ADDR_UNSET,
                       .command = CMD::DISCOVERY,
                       .data = {serial_key[0], serial_key[1]},
                       .data_length = 2};

  } else if (msg.command == CMD::ADDRESSING && received_key == this->self_key) {
    this->self_addr = msg.address;
    return UartMessage{.address = this->self_addr,
                       .command = CMD::ADDRESSING,
                       .data = {serial_key[0], serial_key[1]},
                       .data_length = 2};
  }
  return std::nullopt;
}

std::optional<UartMessage>
SelfNode::initialize_watering(Protocol::FrameDataArray &durations) {
  if (this->get_status() == NodeStatus::INITIALIZING) {
    return std::nullopt;
  }
  Time::Long time_point = this->clock.now();
  std::array<Time::Long, config::node_hose_count> intervals{};
  for (size_t i = 0; i < intervals.size(); i++) {
    time_point += durations[i];
    intervals[i] = time_point;
  }
  this->status = NodeStatus::WATERING;
  this->hose_durations = intervals;
  this->change_active_hose(0);

  return UartMessage{.address = self_addr, .command = CMD::ACK};
}
void SelfNode::process_watering_schedule() {

  if (this->status != NodeStatus::WATERING) {
    return;
  }
  size_t curr_hose = this->active_hose_index;
  Time::Long time_point = this->clock.now();
  if (time_point >= this->hose_durations[curr_hose]) {
    while (curr_hose < config::node_hose_count &&
           time_point >= this->hose_durations[curr_hose]) {
      curr_hose += 1;
    }
    if (curr_hose < config::node_hose_count) {
      this->change_active_hose(curr_hose);
    } else {
      this->conclude_watering();
    }
  }
}
void SelfNode::conclude_watering() {
  this->active_hose_index = HOSE_INACTIVE_IDX;

  Esp_Err_t rc = this->solenoid_manager.deactivate_solenoids();
  if (rc != 0) {
    this->status = NodeStatus::ERR;
  } else {
    this->status = NodeStatus::READY;
  }
}

void SelfNode::complete_initialization() { this->status = NodeStatus::READY; }
void SelfNode::change_active_hose(size_t hose_index) {
  assert(hose_index < config::node_hose_count);
  Esp_Err_t rc = this->solenoid_manager.activate_solenoid(hose_index);
  this->active_hose_index = hose_index;
  if (rc != 0) {
    this->status = NodeStatus::ERR;
  }
}
