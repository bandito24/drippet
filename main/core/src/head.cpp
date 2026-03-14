#include "config.hpp"
#include "driver.hpp"
#include "node.hpp"
#include "protocol.hpp"
#include <assert.h>
#include <cstddef>
#include <head.hpp>
#include <memory>
#include <optional>

Head::Head(iValve &waterFaucetMain, iClock &clock)
    : mainValve(waterFaucetMain), clock(clock) {};
// Need to disregard duplicate broadcast messages from the key
std::optional<config::Address> Head::create_node_pending(NodeKey_t key) {

  if (this->get_node_by_key(key)) { // Is dulicate, ignore
    return std::nullopt;
  }
  // Next address neing max size means its full, handler must hqndle accordingly
  auto next_address = this->get_available_address();
  if (next_address != config::max_nodes) {
    std::unique_ptr<iNode> new_node = std::make_unique<Node>(key);
    this->node_link[next_address] = std::move(new_node);
  }
  return next_address;
}
std::optional<config::Address> Head::get_node_by_key(NodeKey_t key) {
  for (config::Address addr = 0; addr < config::max_nodes; addr++) {
    if (this->node_link[addr] && this->node_link[addr]->get_id_key() == key) {
      return addr;
    }
  }
  return std::nullopt;
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

iNode *Head::get_node(std::size_t index) { return node_link.at(index).get(); }

std::size_t Head::get_node_count() const { return node_count; }

NodeLinkStatus Head::remove_node(std::size_t node_index) {
  if (node_link[node_index] == nullptr) {
    return NodeLinkStatus::LINK_OK;
  };
  node_link[node_index] = nullptr;
  for (int i = node_index; i + 1 < node_count; i++) {
    node_link[i] = std::move(node_link[i + 1]);
  }
  node_count--;
  return LINK_OK;
}
bool Head::has_ready_valves() {
  for (const NodeTypes::Node &node : node_link) {
    if (node->get_node_status() == NodeStatus::READY) {
      return true;
    }
  }
  return false;
}
UartMessage Head::create_watering_frame(config::Address address) {
  NodeTypes::Node &node = this->node_link[address];

  return {.address = address,
          .command = Protocol::Command::INIT_WATER_DURATIONS,
          .data = node->get_all_hose_durations(),
          .data_length = config::node_hose_count};
}

UartMessage Head::create_addressing_frame(uint16_t key,
                                          config::Address address) {

  return {.address = address,
          .command = Protocol::Command::ADDRESSING,
          .data = Protocol::FrameDataArray{key},
          .data_length = 1};
}
UartMessage Head::terminate_endpoint(NodeKey_t key) {
  return {.address = 0x00,
          .command = Protocol::Command::BUGGER_OFF,
          .data = Protocol::FrameDataArray{key},
          .data_length = 1};
}

std::optional<UartMessage> Head::next_watering_frame() {

  for (config::Address addr = 0; addr < config::max_nodes; addr++) {
    NodeTypes::Node &node = this->node_link[addr];
    NodeStatus status = node->get_node_status();

    if (status == NodeStatus::IN_QUEUE) {
      node->set_node_status(NodeStatus::COMMAND_SENT);
      return this->create_watering_frame(addr);
    } else if (status == NodeStatus::COMMAND_SENT) {
      uint8_t retries = node->increase_retry_count();
      if (retries < RETRY_NODE_MAX) {
        node->increase_retry_count();
        return this->create_watering_frame(addr);
      } else {
        this->head_status =
            HeadStatus::FAULTY_NODE; // Will lock valve and require reboot
        node->set_node_status(NodeStatus::UNRESPONSIVE);
        this->mainValve.close_valve();
        return std::nullopt;
      }
    }
  }
  this->head_status = HeadStatus::STANDBY; // No Faulty Nodes and No Nodes to
  return std::nullopt;
}

void Head::initialize_watering_states() {
  for (NodeTypes::Node &node : this->node_link) {
    if (!node) {
      continue;
    }
    NodeStatus status = node->get_node_status();
    if (status == NodeStatus::UNRESPONSIVE) {
      this->head_status = HeadStatus::FAULTY_NODE;
      return;
    }

    if (!node->all_durations_zero() && status == NodeStatus::READY) {
      node->set_node_status(NodeStatus::IN_QUEUE);
    }
  }
  this->head_status = HeadStatus::WATERING_CMDS;
}

using CMD = Protocol::Command;
// tested
std::optional<UartMessage> Head::handle_incoming_frame(UartMessage frame) {

  if (frame.command == CMD::DISCOVERY) {
    // Newly connected Node broadcasts DISCOVERY with key. Head Node
    // responds with ADDRESSING with address and received key (for
    // identification). Node responds with ADDRESSING and key for final
    // acknoledgement
    NodeKey_t key = frame.data[0];
    std::optional<config::Address> address = this->create_node_pending(key);
    if (address) {
      if (address == config::max_nodes) { // Means system has max nodes
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
  } else {
    printf("unknown command of %d", static_cast<int>(frame.command));
  }
  return std::nullopt;
}
void Head::process_watering_schedule() {

  if (this->is_watering_due()) {
    if (this->head_status == HeadStatus::STANDBY) {
      this->initialize_watering_states(); // For head status and nodes
    }
    this->progress_watering_due();
  }
}
