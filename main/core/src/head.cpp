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
    : mainValve(waterFaucetMain), clock(clock){};
// Need to disregard duplicate broadcast messages from the key
std::optional<config::Address> Head::create_node_pending(NodeKey_t key) {

  if (this->get_node_by_key(key)) {
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
