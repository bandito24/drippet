#include "clock.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "node.hpp"
#include "protocol.hpp"
#include "util.hpp"
#include <assert.h>
#include <chrono>
#include <cstddef>
#include <head.hpp>
#include <memory>
#include <numeric>
#include <optional>

std::optional<config::Address> Head::create_node_pending(NodeKey_t key) {

  std::optional<size_t> exising_addr = this->get_node_by_key(key);
  if (exising_addr) { // Has already requested, send back addr used
    auto node = this->get_node(*exising_addr);
    if (node->get_node_status() == NodeStatus::INITIALIZING) {
      return *exising_addr;
    }
    return std::nullopt; // Means its a duplicate request
  }
  // Next address neing max size means its full, handler must hqndle accordingly
  auto next_address = this->get_available_address();
  if (next_address != config::max_nodes) {
    std::unique_ptr<iNode> new_node = std::make_unique<Node>(key);
    this->node_link[next_address] = std::move(new_node);
    // Reads from nvs storage--if not previously set then is just zeros
    this->node_link[next_address]->set_node_durations(
        this->storage.read_boot_durations(next_address));
  }
  return next_address;
}
std::optional<config::Address> Head::get_node_by_key(NodeKey_t key) {
  for (config::Address addr = 0; addr < config::max_nodes; addr++) {
    NodeTypes::Node &node = this->node_link[addr];
    if (node && node->get_id_key() == key) {
      return addr;
    }
  }
  return std::nullopt;
}

bool Head::is_node_watering_today(size_t addr, Weekdays todays_day) {
  auto node = this->get_node(addr);
  if (!node) {
    return false;
  }
  size_t day_index = static_cast<size_t>(todays_day);
  NodeTypes::WateringSchedule schedule = node->get_weekly_waterings();
  return schedule[day_index];
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

iNode *Head::get_node(std::size_t index) {
  if (index >= config::max_nodes) {
    return nullptr;
  }
  return node_link.at(index).get();
}

std::size_t Head::get_node_count() const { return node_count; }

UartMessage Head::create_watering_frame(config::Address address) {
  assert(this->get_node(address) != nullptr);
  NodeTypes::Node &node = this->node_link[address];

  return {.address = address,
          .command = Protocol::Command::INIT_WATER_DURATIONS,
          .data = node->get_all_hose_durations(),
          .data_length = config::node_hose_count};
}

UartMessage Head::create_status_frame(config::Address address) const {
  return {.address = address, .command = Protocol::Command::STATUS};
}

UartMessage Head::create_addressing_frame(uint16_t key,
                                          config::Address address) {

  return {.address = address,
          .command = Protocol::Command::ADDRESSING,
          .data = Protocol::FrameDataArray{key},
          .data_length = 1};
}
UartMessage Head::terminate_endpoint(NodeKey_t key) {
  return {.address = ADDR_UNSET,
          .command = Protocol::Command::BUGGER_OFF,
          .data = Protocol::FrameDataArray{key},
          .data_length = 1};
}

std::optional<UartMessage> Head::next_watering_frame() {
  if (this->head_status != HeadStatus::WATERING_CMDS) {
    return std::nullopt;
  }
  if (!this->active_watering_index ||
      !this->get_node(*this->active_watering_index)) {
    this->head_status = HeadStatus::STANDBY;
    this->active_watering_index = std::nullopt;
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
      this->head_status =
          HeadStatus::FAULTY_NODE; // Will lock valve and require reboot
      node->set_node_status(NodeStatus::ERR);
      this->mainValve.close_valve();
      return std::nullopt;
    }
  }

  this->head_status = HeadStatus::STANDBY;
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
    Weekdays curr_day = this->clock.get_day_of_week();

    if (!node->all_durations_zero() && status == NodeStatus::READY &&
        this->is_node_watering_today(i, curr_day)) {
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
    // Newly connected Node broadcasts DISCOVERY with key. Head Node
    // responds with ADDRESSING with address and received key (for
    // identification). Node responds with ADDRESSING and key for final
    // acknoledgement
    NodeKey_t key = frame.data[0];
    std::optional<config::Address> address = this->create_node_pending(key);
    if (address) {
      if (address >= config::max_nodes) { // Means system has max nodes
        return this->terminate_endpoint(key);
      } else {
        return this->create_addressing_frame(key, *address);
      }
    } else {
      // No Address means key is recognized and it's a duplicate request
      return std::nullopt;
    }
  } else if (frame.command ==
             CMD::ADDRESSING) { // Means the node is attempting to reply
                                // with the same address and key for final
                                // confirmation
    NodeKey_t key = frame.data[0];
    NodeLinkStatus status = this->confirm_node_pending(key, frame.address);
    if (status == NodeLinkStatus::LINK_OK) {
      Logger::log_simple("Node Successfully connected");
    } else {
      Logger::log_error("Node connect unsuccessful");
    }
    // If unsuccessful do nothing and node will reattempt the broadcast
    // after short period
  } else if (frame.command ==
             CMD::ACK) { // Only received after sending a watering command.
                         // Nothing to do after.
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
  assert(frame.data[0] < static_cast<int>(NodeStatus::NODE_NONEXISTANT));
  NodeStatus status = static_cast<NodeStatus>(frame.data[0]);
  this->set_node_status(frame.address, status);
  switch (status) {
  case NodeStatus::READY: // Sends ready only after completing the watering
    this->advance_active_watering_index();
    break;
  case NodeStatus::ERR:
    this->head_status = HeadStatus::FAULTY_NODE;
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
  this->active_watering_index = std::nullopt;
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
ActionStatus
Head::set_node_durations(size_t index,
                         const NodeTypes::HoseDurations &durations) {
  auto node = this->get_node(index);
  if (!node) {
    return ActionStatus::INVALID_NODE;
  }
  int new_time_pool = this->calculate_new_time_pool(index, durations);
  if (new_time_pool < 0) {
    NodeTypes::HoseDurations empty{};
    if (durations != empty) { // Don't remove the infinite loop protection
      this->set_node_durations(index, empty);
    }
    node->set_node_status(NodeStatus::INVALID_TIME);
    return ActionStatus::INVALID_TIME;
  }
  ActionStatus rc = node->set_node_durations(durations);
  if (rc == ActionStatus::OK) {
    this->time_pool = new_time_pool;
    this->storage.save_durations(index, durations,
                                 node->get_weekly_waterings());
  }
  return rc;
}

ActionStatus
Head::set_weekly_waterings(size_t index,
                           const NodeTypes::WateringSchedule &schedule) {

  auto node = this->get_node(index);
  if (!node) {
    return ActionStatus::INVALID_NODE;
  }

  node->set_weekly_waterings(schedule);

  this->storage.save_durations(index, node->get_all_hose_durations(),
                               node->get_weekly_waterings());
  return ActionStatus::OK;
}

int Head::calculate_new_time_pool(
    size_t index, const NodeTypes::HoseDurations &new_durations) {
  int duration_pool = config::MAX_WATERING_SECONDS;
  for (size_t addr = 0; addr < config::max_nodes; addr++) {
    int sum = 0;
    if (addr == index) {
      sum = std::accumulate(new_durations.begin(), new_durations.end(), 0);
    } else {
      iNode *node = this->get_node(addr);
      if (node) {
        const NodeTypes::HoseDurations &durations =
            node->get_all_hose_durations();
        sum = std::accumulate(durations.begin(), durations.end(), 0);
      }
    }
    duration_pool -= sum;
  }
  return duration_pool;
}

std::optional<NodeTypes::HoseDurations>
Head::get_node_hose_durations(size_t addr) {

  auto node = this->get_node(addr);
  if (node) {
    return node->get_all_hose_durations();
  } else {
    return std::nullopt;
  }
}
all_durations_t Head::retrieve_all_durations() {
  all_durations_t all_durs{};
  for (size_t addr = 0; addr < config::max_nodes; addr++) {
    auto durations = this->get_node_hose_durations(addr);
    all_durs.at(addr) =
        durations ? durations.value()
                  : std::array<Time::Time_Seconds, config::node_hose_count>{};
  }
  return all_durs;
}
void Head::print_node_durations() {
  for (size_t i = 0; i < config::max_nodes; i++) {
    auto node = this->get_node(i);
    if (node) {
      node->print_hose_durations(i);
    } else {
      printf("Node %d not initialized\n", i);
    }
  }
}
