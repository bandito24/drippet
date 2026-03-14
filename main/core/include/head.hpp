#pragma once
#include "config.hpp"
#include "logger.hpp"
#include "protocol.hpp"
#include <array>
#include <node.hpp>
#include <optional>

enum NodeLinkStatus {
  LINK_OK,
  LINK_FULL,
  LINK_INVALID_INDEX,
  LINK_KEY_MISMATCH,
  LINK_ERR_ADDRESS,
  LINK_ERR

};

constexpr uint8_t RETRY_NODE_MAX = 10;
enum class HeadStatus { STANDBY, FAULTY_NODE, WATERING_CMDS };
enum ValveStatus { VALVE_OPEN, VALVE_CLOSED };
struct PendingNode {
  config::Address address{};
  NodeKey_t key{};
};

class Head {
private:
  iValve &mainValve;
  iClock &clock;
  std::array<NodeTypes::Node, config::max_nodes> node_link{};
  std::size_t node_count = 0;
  ValveStatus valve_status = VALVE_CLOSED;
  bool has_ready_valves();
  std::optional<config::Address> get_node_by_key(NodeKey_t key);

  void initialize_watering_states();

  void progress_watering_due() { return this->clock.progress_watering_due(); }
  HeadStatus head_status =
      HeadStatus::STANDBY; // only is changed by initialize_watering_procedure
                           // ane generate_next_watering_command

public:
  HeadStatus get_head_status() { return this->head_status; };
  static constexpr std::size_t max_nodes = config::max_nodes;
  Head(iValve &waterFaucetMain, iClock &clock);
  NodeLinkStatus remove_node(std::size_t node_index);
  iNode *get_node(std::size_t node_index);
  std::size_t get_node_count() const;
  std::optional<config::Address> create_node_pending(NodeKey_t key);
  NodeLinkStatus confirm_node_pending(NodeKey_t key, config::Address position);
  bool is_watering_due() const { return this->clock.is_watering_due(); }
  std::optional<UartMessage> next_watering_frame();

  config::Address get_available_address();

  UartMessage create_addressing_frame(uint16_t key, config::Address address);
  UartMessage create_watering_frame(config::Address address);

  UartMessage terminate_endpoint(NodeKey_t key);
  std::optional<UartMessage> handle_incoming_frame(UartMessage msg);
  void process_watering_schedule();
};
