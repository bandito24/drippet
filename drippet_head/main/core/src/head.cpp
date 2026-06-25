#include "clock.hpp"
#include "config.hpp"
#include "conn_context.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "gatt_attribute.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "protocol.hpp"
#include "self_node.hpp"
#include "util.hpp"
#include <assert.h>
#include <config.hpp>
#include <cstddef>
#include <head.hpp>
#include <memory>
#include <numeric>
#include <optional>

std::optional<config::Address> Head::create_node_pending(NodeKey_t key) {

  std::optional<size_t> exising_addr = this->get_node_addr_by_key(key);
  if (exising_addr) { // Has already requested, send back addr used
    return *exising_addr;
  }
  // Next address neing max size means its full, handler must hqndle accordingly
  auto next_address = this->get_available_address();
  if (next_address == config::max_nodes) {
    return std::nullopt;
  }
  std::unique_ptr<iNode> new_node = std::make_unique<Node>(key);
  this->node_link[next_address] = std::move(new_node);
  // Reads from nvs storage--if not previously set then is just zeros
  this->node_link[next_address]->set_node_durations(
      this->storage.read_boot_durations(next_address));
  return next_address;
}
std::optional<config::Address> Head::get_node_addr_by_key(NodeKey_t key) {
  for (config::Address addr = 0; addr < config::max_nodes; addr++) {
    NodeTypes::Node &node = this->node_link[addr];
    if (node && node->get_id_key() == key) {
      return addr;
    }
  }
  return std::nullopt;
}

bool Head::is_node_watering_this_phase(size_t addr) {
  auto node = this->get_node(addr);
  if (!node) {
    return false;
  }
  CyclePhase phase = this->clock.get_phase_of_cycle();
  size_t phase_index = static_cast<size_t>(phase);
  NodeTypes::WateringCycle cycle = node->get_watering_cycle();
  return cycle[phase_index] && node->get_hose_duration() != 0;
}

NodeLinkStatus Head::confirm_node_pending(NodeKey_t key,
                                          config::Address address) {
  if (address >= this->node_link.size()) {
    return NodeLinkStatus::LINK_INVALID_INDEX;
  }
  NodeTypes::Node &confirmed_node = this->node_link[address];
  assert(confirmed_node != nullptr);
  if (confirmed_node->get_id_key() != key) {
    return NodeLinkStatus::LINK_KEY_MISMATCH;
  }
  confirmed_node->set_node_status(NodeStatus::READY);
  return NodeLinkStatus::LINK_OK;
}

config::Address Head::get_available_address() {

  for (size_t i = 0; i < config::max_nodes; i++) {
    if (this->node_link[i] == nullptr) {
      return static_cast<config::Address>(i);
    }
  }
  // Means it's full and handler must handle accordingly
  return static_cast<config::Address>(config::max_nodes);
}

iNode *Head::get_node(std::size_t index) const {
  if (index >= config::max_nodes) {
    return nullptr;
  }
  return node_link.at(index).get();
}

UartMessage Head::create_watering_frame(config::Address address) {
  assert(this->get_node(address) != nullptr);
  NodeTypes::Node &node = this->node_link[address];

  return {.address = address,
          .command = Protocol::Command::INIT_WATER_DURATIONS,
          .data = {node->get_hose_duration()},
          .data_length = 1};
}

UartMessage Head::create_status_frame(config::Address address) const {
  return {.address = address, .command = Protocol::Command::STATUS};
}

UartMessage Head::create_addressing_frame(NodeKey_t key,
                                          config::Address address) {

  auto ser_key = Util::serialize_key(key);
  return {.address = address,
          .command = Protocol::Command::ADDRESSING,
          .data = Protocol::FrameDataArray{ser_key[0], ser_key[1]},
          .data_length = 2};
}

std::optional<UartMessage> Head::next_watering_frame() {
  if (this->head_status != HeadStatus::WATERING_CMDS) {
    return std::nullopt;
  }
  if (!this->active_watering_index ||
      !this->get_node(*this->active_watering_index)) {
    this->conclude_watering(HeadStatus::STANDBY);
    return std::nullopt;
  }
  auto node = this->get_node(*this->active_watering_index);
  size_t active_idx = *this->active_watering_index;
  NodeStatus status = node->get_node_status();

  if (status == NodeStatus::IN_QUEUE) {
    node->set_node_status(NodeStatus::COMMAND_SENT);
    return this->create_watering_frame(active_idx);
  } else if (status == NodeStatus::COMMAND_SENT ||
             status == NodeStatus::WATERING) {
    uint8_t retries = node->increase_retry_count();
    if (retries < RETRY_NODE_MAX) {
      if (status == NodeStatus::COMMAND_SENT) {
        return this->create_watering_frame(active_idx);
      } else if (status == NodeStatus::WATERING) {
        return this->create_status_frame(active_idx);
      }
    } else {
      node->set_node_status(NodeStatus::ERR);
      this->conclude_watering(HeadStatus::FAULTY_NODE);
      return std::nullopt;
    }
  }

  this->conclude_watering(HeadStatus::STANDBY);
  return std::nullopt;
}
void Head::ack_node_watering_confirmation(config::Address addr) {
  iNode *node = this->get_node(addr);
  assert(node != nullptr && addr < config::max_nodes);
  node->set_node_status(NodeStatus::WATERING);
  node->clear_retry_count();
}

void Head::initialize_watering_states() {
  for (size_t i = 0; i < this->node_link.size(); i++) {
    auto node = this->get_node(i);
    if (!node) {
      continue;
    }
    NodeStatus status = node->get_node_status();

    if (status == NodeStatus::ERR) {
      this->head_status = HeadStatus::FAULTY_NODE;
      return;
    }

    if (status == NodeStatus::READY && this->is_node_watering_this_phase(i)) {
      node->set_node_status(NodeStatus::IN_QUEUE);
      if (!this->active_watering_index) {
        this->active_watering_index = i;
      }
    }
  }
  this->head_status = HeadStatus::WATERING_CMDS;
}

using CMD = Protocol::Command;

// This method is in charge of updating node state in response to messages
//  Next_waterin_frame is in change of reacting to the updated node state
std::optional<UartMessage>
Head::handle_incoming_frame(const UartMessage &frame) {

  if (frame.command == CMD::DISCOVERY) {
    NodeKey_t key = Util::deserialize_key(frame.data);
    std::optional<config::Address> address = this->create_node_pending(key);
    if (address && address < config::max_nodes) {
      return this->create_addressing_frame(key, *address);
    }
  } else if (frame.command == CMD::ADDRESSING) {
    NodeKey_t key = Util::deserialize_key(frame.data);
    NodeLinkStatus status = this->confirm_node_pending(key, frame.address);
    // If status is bad, try creating again. Node can adjust addr until ack
    if (status != NodeLinkStatus::LINK_OK) {
      std::optional<config::Address> address = this->create_node_pending(key);
      if (address) {
        return this->create_addressing_frame(key, *address);
      }
    }
    return UartMessage{frame.address, Protocol::Command::ACK};
  } else if (frame.command == CMD::ACK) {
    this->ack_node_watering_confirmation(frame.address);
  } else if (frame.command == CMD::STATUS) {
    this->handle_incoming_node_status(frame);
  }

  else {
    printf("unknown command of %d", static_cast<int>(frame.command));
  }
  return std::nullopt;
}

void Head::handle_incoming_node_status(const UartMessage &frame) {

  // Node will only send values , READY (completed, watering, or err)
  // We only react to two of them
  NodeStatus status = static_cast<NodeStatus>(frame.data[0]);
  config::Address addr = static_cast<config::Address>(frame.address);

  this->get_node(frame.address)->clear_retry_count();
  this->set_node_status(frame.address, status);
  switch (status) {
  case NodeStatus::READY: // Sends ready only after completing the watering
    if (addr == this->get_active_watering_index()) {
      this->advance_active_watering_index();
    }
    break;
  case NodeStatus::WATERING:
    // Do nothing
    break;
  case NodeStatus::ERR:
    this->conclude_watering(HeadStatus::FAULTY_NODE);
    break;
  default:
    Logger::log_error("Invalid incoming node status value of %d",
                      frame.data[0]);
    break;
  }
}

void Head::advance_active_watering_index() {
  size_t starting_point;
  if (!this->active_watering_index) {
    starting_point = 0;
  } else {
    starting_point = *this->active_watering_index;
  }
  for (size_t i = starting_point; i < this->node_link.size(); i++) {
    auto status = this->get_node_status(i);
    if (status == NodeStatus::IN_QUEUE) {
      this->active_watering_index = i;
      return;
    }
  }
  this->conclude_watering(HeadStatus::STANDBY);
}

void Head::process_watering_schedule() {

  if (this->is_watering_due()) {

    if (this->head_status == HeadStatus::STANDBY) {

      this->initialize_watering_states(); // For head status and nodes
    }
    this->progress_watering_due();
  }
}

all_node_status_t Head::get_node_statuses() {
  all_node_status_t result{};

  for (size_t addr = 0; addr < config::max_nodes; addr++) {
    iNode *node = this->get_node(addr);
    if (!node) {
      result[addr] = static_cast<uint8_t>(NodeStatus::NODE_NONEXISTANT);
    } else {
      result[addr] = static_cast<uint8_t>(node->get_node_status());
    }
  }
  return result;
}

void Head::set_node_status(size_t index, NodeStatus status) {
  auto node = this->get_node(index);
  if (node) {
    node->set_node_status(status);
  }
}
ActionStatus Head::set_node_duration(size_t index,
                                     NodeTypes::HoseDuration duration) {
  auto node = this->get_node(index);
  if (!node) {
    return ActionStatus::INVALID_NODE;
  }
  int new_time_pool = this->calculate_new_time_pool(index, duration);
  if (new_time_pool < 0) {
    if (duration != 0) { // Don't remove the infinite loop protection
      this->set_node_duration(index, 0);
    }
    node->set_node_status(NodeStatus::INVALID_TIME);
    return ActionStatus::INVALID_TIME;
  }
  ActionStatus rc = node->set_node_duration(duration);
  if (rc == ActionStatus::OK) {
    this->time_pool = new_time_pool;
    this->storage.save_durations(index, duration, node->get_watering_cycle());
  }
  return rc;
}

ActionStatus
Head::set_watering_cycle(size_t index,
                         const NodeTypes::WateringCycle &schedule) {

  auto node = this->get_node(index);
  if (!node) {
    return ActionStatus::INVALID_NODE;
  }

  node->set_watering_cycle(schedule);

  this->storage.save_durations(index, node->get_hose_duration(),
                               node->get_watering_cycle());
  return ActionStatus::OK;
}

int Head::calculate_new_time_pool(size_t index,
                                  NodeTypes::HoseDuration new_duration) {
  int duration_pool = this->phase_length;
  for (size_t addr = 0; addr < config::max_nodes; addr++) {
    int sum = 0;
    if (addr == index) {
      sum = new_duration;
    } else {
      iNode *node = this->get_node(addr);
      if (node) {
        sum = node->get_hose_duration();
      }
    }
    duration_pool -= sum;
  }
  return duration_pool;
}

std::optional<NodeTypes::DurationSchedule>
Head::get_node_duration_schedule(size_t addr) const {

  auto node = this->get_node(addr);
  if (node) {
    return node->get_duration_schedule();
  } else {
    return std::nullopt;
  }
}

std::optional<NodeTypes::HoseDuration>
Head::get_node_hose_duration(size_t addr) {

  auto node = this->get_node(addr);
  if (node) {
    return node->get_hose_duration();
  } else {
    return std::nullopt;
  }
}

void Head::print_node_durations() {
  for (size_t i = 0; i < config::max_nodes; i++) {
    auto node = this->get_node(i);
    if (node) {
      node->print_hose_duration(i);
    } else {
      Logger::log_simple("Node %d not initialized\n", static_cast<int>(i));
    }
  }
}

// This should only be called at the very end/beginning of the head_tqsk loop
// TEST: This functionality
void Head::process_external_requests() {
  OptionalRequest opt = this->extRequestsManager.pop();
  while (opt.has_value()) {
    ExtRequest req = *opt;
    switch (req.type) {
      // Queued by ext_req_pairing_mode
    case Req_t::INIT_PAIRING:
      this->init_pairing_mode();
      break;

      // Queued by ext_req_set_node_duration
    case Req_t::MODIFY_NODE_DURATIONS:
      this->set_node_duration(req.data[0], req.data[1]);
      break;

      // Queued by ext_req_set_node_cycle
    case Req_t::MODIFY_NODE_CYCLE: {
      NodeTypes::WateringCycle wc = Util::byte_to_water_cycle(req.data[1]);
      this->set_watering_cycle(req.data[0], wc);
      break;
    }

      // Queued by ext_req_set_phase
    case Req_t::MODIFY_PHASE_START_TIME:
      this->clock.set_next_phase_start_time(req.data[0], req.data[1]);
      break;

      // Queued by ext_req_set_clock
    case Req_t::MODIFY_CLOCK_TIME:
      this->clock.set_time(2026, 12, 12, req.data[0], req.data[1]);
      break;

    default:
      Logger::log_error("Invalid external request of %d", req.type);
      break;
    }
    opt = this->extRequestsManager.pop();
  }
}

void Head::init_pairing_mode() {

  Logger::log_simple("Initializing pairing mode");
  this->conclude_watering(HeadStatus::PAIRING);
  for (size_t i = 0; i < config::max_nodes; i++) {
    this->node_link.at(i) = nullptr;
  }

  this->time_pool = this->phase_length;
  // TODO: test this so that its definitely called
  //
  Logger::log_simple("About to kill node task function");
  this->kill_node_task_fn();
}

OptMsg Head::process_pairing(OptMsg response, uint32_t tick_key) {
  if (this->head_status != HeadStatus::PAIRING) {
    return std::nullopt;
  }

  if (this->no_reponse_pairing_count >= STOP_PAIRING_COUNT) {
    this->end_pairing_mode();
  } else {
    if (!response) {
      this->no_reponse_pairing_count++;
      auto key = Util::serialize_key(tick_key);
      return UartMessage{.address = ADDR_UNSET,
                         .command = CMD::DISCOVERY,
                         .data = {key[0], key[1]}};

    } else {
      this->no_reponse_pairing_count = 0;
    }
  }
  return std::nullopt;
}
