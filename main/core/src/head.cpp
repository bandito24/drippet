#include "config.hpp"
#include "node.hpp"
#include "protocol.hpp"
#include <assert.h>
#include <cstddef>
#include <head.hpp>
#include <memory>

Head::Head(iValve &waterFaucetMain, iClock &clock)
    : mainValve(waterFaucetMain), clock(clock){};

std::optional<config::Address> Head::create_node_pending(NodeKey_t key) {
  auto next_address = this->get_available_address();
  if (next_address) {
    std::unique_ptr<iNode> new_node =
        std::make_unique<Node>(*next_address, key);
    this->node_link[*next_address] = std::move(new_node);
  }
  return next_address;
}

// TODO:: Make sure the address and key are confirmed
NodeLinkStatus Head::confirm_node_pending(NodeKey_t key,
                                          config::Address address) {
  if (address >= this->node_link.size()) {
    return NodeLinkStatus::LINK_INVALID_INDEX;
  }
  NodeTypes::Node_Link &confirmed_node = this->node_link[address];
  assert(confirmed_node != nullptr);
  confirmed_node->set_node_status(NodeStatus::READY);
  return NodeLinkStatus::LINK_OK;
}

std::optional<config::Address> Head::get_available_address() {

  for (size_t i = 0; i < config::max_nodes; i++) {
    if (this->node_link[i] == nullptr) {
      return static_cast<config::Address>(i);
    }
  }
  return std::nullopt;
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
void Head::poll_nodes() {
  Time::Time_Point current_timepoint = this->clock.now();
  for (std::size_t i = 0; i < this->node_count; i++) {
    if (node_link[i]->get_node_status() == NodeStatus::IDLE &&
        node_link[i]->next_water_timepoint() < current_timepoint) {
      node_link[i]->set_node_status(NodeStatus::READY);
    }
  }
}
bool Head::has_ready_valves() {
  for (const NodeTypes::Node_Link &node : node_link) {
    if (node->get_node_status() == NodeStatus::READY) {
      return true;
    }
  }
  return false;
}
void Head::update() {

  if (this->valve_status == ValveStatus::VALVE_CLOSED) {
    if (has_ready_valves()) {
      mainValve.open_valve();
    }
  } else if (this->valve_status == ValveStatus::VALVE_OPEN) {
    if (has_ready_valves()) {
      for (const NodeTypes::Node_Link &node : node_link) {
        if (node->get_node_status() == NodeStatus::READY) {
          node->send_node_directions();
          node->set_node_status(NodeStatus::COMMAND_SENT);
        }
      }
    } else {
      mainValve.close_valve();
    }
  }
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
