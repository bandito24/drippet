#include "uart.hpp"
#include <array>
#include <assert.h>
#include <config.hpp>
#include <cstddef>
#include <head.hpp>
#include <node.hpp>
#include <time.hpp>

Head::Head(iValve &waterFaucetMain, iClock &clock)
    : mainValve(waterFaucetMain), clock(clock){};

NodeLinkStatus Head::add_node(NodeTypes::Node_Link node) {
  if (this->node_count == config::max_nodes) {
    return LINK_FULL;
  }

  assert(node_count < config::max_nodes);
  assert(node_link[node_count] == nullptr);
  config::Address new_address = node->get_address();
  for (NodeTypes::Node_Link &node : node_link) {
    if (node == nullptr)
      continue;
    if (node->get_address() == new_address) {
      node->set_node_status(NodeStatus::ERR);
      return NodeLinkStatus::LINK_ERR_ADDRESS;
    }
  }

  this->node_link[this->node_count] = std::move(node);
  node_count++;
  return LINK_OK;
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
UartMessage Head::create_addressing_frame(uint16_t key) {
  uint8_t new_index_address = this->node_count + 1;

  return {.address = new_index_address,
          .command = Protocol::Command::ADDRESSING,
          .data = Protocol::FrameDataArray{key},
          .data_length = 1};
}
