#pragma once
#include "clock.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "protocol.hpp"
#include "storage.hpp"
#include "switch.hpp"
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
constexpr uint32_t INITIAL_TIME_POOL = config::WATER_INTERVAL;
enum class HeadStatus { PAIRING, STANDBY, FAULTY_NODE, WATERING_CMDS };
enum ValveStatus { VALVE_OPEN, VALVE_CLOSED };
using all_node_status_t = std::array<uint8_t, config::max_nodes>;

class Head {
private:
  Switch &mainValve;
  iClock &clock;
  Storage &storage;
  Time::Long time_pool = config::WATER_INTERVAL;
  std::array<NodeTypes::Node, config::max_nodes> node_link{};
  void advance_active_watering_index();
  std::size_t node_count = 0;
  std::optional<size_t> active_watering_index = std::nullopt;

  void initialize_watering_states();

  void progress_watering_due() { return this->clock.progress_watering_due(); }
  HeadStatus head_status =
      HeadStatus::STANDBY; // only is changed by initialize_watering_procedure
                           // ane generate_next_watering_command
                           //
  void ack_node_watering_confirmation(config::Address addr);
  void conclude_watering(HeadStatus resolve_status) {
    this->active_watering_index = std::nullopt;
    this->mainValve.disable();
    this->head_status = resolve_status;
  }
  iNode *get_node(std::size_t node_index);

public:
  Head(Switch &waterFaucetMain, iClock &clock, Storage &store)
      : mainValve{waterFaucetMain}, clock{clock}, storage{store} {
    mainValve.init();
  };
  std::optional<size_t> get_active_watering_index() const {
    return this->active_watering_index;
  }
  HeadStatus &get_head_status() { return this->head_status; };
  const HeadStatus &read_node_status() const { return this->head_status; }
  static constexpr std::size_t max_nodes = config::max_nodes;
  std::size_t get_node_count() const;
  std::optional<config::Address> create_node_pending(NodeKey_t key);
  NodeLinkStatus confirm_node_pending(NodeKey_t key, config::Address position);

  std::optional<config::Address> get_node_by_key(NodeKey_t key);
  bool is_watering_due() const { return this->clock.is_watering_due(); }
  std::optional<UartMessage> next_watering_frame();
  all_node_status_t get_node_statuses();
  void set_node_status(size_t index, NodeStatus status);
  ActionStatus set_node_durations(size_t index,
                                  const NodeTypes::HoseDurations &durations);
  void init_pairing_mode();
  void end_pairing_mode() { this->head_status = HeadStatus::STANDBY; }

  ActionStatus
  set_weekly_waterings(size_t index,
                       const NodeTypes::WateringSchedule &schedule);
  NodeStatus get_node_status(size_t addr) {
    if (this->get_node(addr)) {
      return this->get_node(addr)->get_node_status();
    } else {
      return NodeStatus::NODE_NONEXISTANT;
    }
  }
  bool node_exists(size_t addr) { return this->get_node(addr) != nullptr; }

  std::optional<NodeTypes::HoseDurations> get_node_hose_durations(size_t index);

  config::Address get_available_address();

  UartMessage create_addressing_frame(NodeKey_t key, config::Address address);
  UartMessage create_watering_frame(config::Address address);

  UartMessage create_status_frame(config::Address address) const;

  int calculate_new_time_pool(size_t index,
                              const NodeTypes::HoseDurations &new_durations);
  bool is_node_watering_today(size_t addr, Weekdays todays_day);

  UartMessage terminate_endpoint(NodeKey_t key);
  std::optional<UartMessage> handle_incoming_frame(const UartMessage &msg);
  void process_watering_schedule();
  int get_node_retry_count(size_t addr) {
    if (this->get_node(addr)) {
      return this->get_node(addr)->get_retry_count();
    } else {
      return -1;
    }
  }
  all_durations_t retrieve_all_durations();
  void print_node_durations();
  void handle_incoming_node_status(const UartMessage &frame);
};
