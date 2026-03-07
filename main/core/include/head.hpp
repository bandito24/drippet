#pragma once
#include "config.hpp"
#include "protocol.hpp"
#include <array>
#include <node.hpp>
#include <time.hpp>

enum NodeLinkStatus {
  LINK_OK,
  LINK_FULL,
  LINK_INVALID_INDEX,
  LINK_ERR_ADDRESS,
  LINK_ERR
};
enum ValveStatus { VALVE_OPEN, VALVE_CLOSED };
struct PendingNode {
  config::Address address{};
  NodeKey_t key{};
};

class Head {
private:
  iValve &mainValve;
  iClock &clock;
  std::array<NodeTypes::Node_Link, config::max_nodes> node_link{};
  std::size_t node_count = 0;
  ValveStatus valve_status = VALVE_CLOSED;
  bool has_ready_valves();

public:
  static constexpr std::size_t max_nodes = config::max_nodes;
  Head(iValve &waterFaucetMain, iClock &clock);
  NodeLinkStatus remove_node(std::size_t node_index);
  iNode *get_node(std::size_t node_index);
  std::size_t get_node_count() const;
  PendingNode pending_node = {};
  std::optional<config::Address> create_node_pending(NodeKey_t key);
  NodeLinkStatus confirm_node_pending(NodeKey_t key, config::Address position);

  std::optional<config::Address> get_available_address();
  void reset_pending_node() { pending_node = {}; }

  UartMessage create_addressing_frame(uint16_t key, config::Address address);

  UartMessage terminate_endpoint(NodeKey_t key);

  void poll_nodes();
  void update();
};
