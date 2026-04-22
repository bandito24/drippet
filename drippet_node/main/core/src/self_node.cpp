#include "self_node.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "logger.hpp"
#include "protocol.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

using CMD = Protocol::Command;
std::optional<UartMessage> SelfNode::handle_incoming_frame(UartMessage &msg) {

  if (msg.command == CMD::ADDRESSING || msg.command == CMD::BUGGER_OFF) {
    // These are the only possible messages one can receive without addressing.
    // Key is used for endpoint
    this->update_last_evt_time();
    NodeKey_t key = msg.data[0];
    if (this->has_addr() || this->self_key != key) {
      return std::nullopt;
    }
    if (msg.command == CMD::ADDRESSING) {
      this->self_addr = msg.address;
      return UartMessage{.address = this->self_addr,
                         .command = CMD::ADDRESSING,
                         .data = {this->self_key},
                         .data_length = 1};
    } else if (msg.command == CMD::BUGGER_OFF) {
      this->deactivate = true;
    }
  }
  if (!this->has_addr() || msg.address != this->self_addr) {
    return std::nullopt;
  }
  // At this point we know its a certified command;
  this->update_last_evt_time();
  switch (msg.command) {
  case CMD::ACK: // Only received after sending back key and address and being
                 // recognizaed by head
    this->complete_initialization();
    break;
  case CMD::INIT_WATER_DURATIONS:
    this->initialize_watering(msg.data);
    return UartMessage{.address = self_addr, .command = CMD::ACK};
  default: {
    Logger::log_error("Invalid command of %d", msg.command);
    return std::nullopt;
  }
  }
  return std::nullopt;
}

void SelfNode::initialize_watering(Protocol::FrameDataArray &durations) {
  for (auto &duration : durations) {
    // Head will be validating this, just a final failsafe
    if (duration > config::MAX_HOSE_DURATION) {
      duration = config::MAX_HOSE_DURATION;
    }
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
}
void SelfNode::process_watering_schedule() {

  size_t curr_hose = this->active_hose_index;
  assert(curr_hose < config::node_hose_count);
  Time::Long time_point = this->clock.now();
  if (time_point > this->hose_durations[curr_hose]) {
    size_t next_index = curr_hose + 1;
    if (next_index < config::node_hose_count) {
      this->change_active_hose(next_index);
    } else {
      this->conclude_watering();
    }
  }
}
void SelfNode::conclude_watering() {
  this->active_hose_index = config::node_hose_count;
  this->status = NodeStatus::READY;
}

UartMessage SelfNode::generate_discovery_message() {
  assert(this->self_addr == ADDR_UNSET);
  return {.address = ADDR_UNSET,
          .command = Protocol::Command::DISCOVERY,
          .data = Protocol::FrameDataArray{this->self_key},
          .data_length = 1};
}
void SelfNode::complete_initialization() { this->status = NodeStatus::READY; }
void SelfNode::change_active_hose(size_t hose_index) {
  assert(hose_index < config::node_hose_count);
  RC rc = RC::OK;
  this->active_hose_index = hose_index;
  if (rc != RC::OK) {
    this->status = NodeStatus::ERR;
  }
}
