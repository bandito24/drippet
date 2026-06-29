#pragma once
#include "clock.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "external_requests.hpp"
#include "logger.hpp"
#include "protocol.hpp"
#include "self_node.hpp"
#include "storage.hpp"
#include "switch.hpp"
#include <array>
#include <functional>
#include <memory>
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

constexpr int STOP_PAIRING_COUNT = 10;
constexpr uint8_t RETRY_NODE_MAX = 10;
enum class HeadStatus { PAIRING, STANDBY, FAULTY_NODE, WATERING_CMDS };
enum ValveStatus { VALVE_OPEN, VALVE_CLOSED };
using all_node_status_t = std::array<uint8_t, config::max_nodes>;

class Head {
private:
  iClock &clock;
  Storage &storage;
  const Time::Long phase_length;
  Time::Long time_pool;
  std::array<NodeTypes::Node, config::max_nodes> node_link{};
  void advance_active_watering_index();
  std::optional<size_t> active_watering_index = std::nullopt;
  std::function<void()> kill_node_task_fn;

  void initialize_watering_states();

  void progress_watering_due() { return this->clock.progress_watering_due(); }
  HeadStatus head_status =
      HeadStatus::STANDBY; // only is changed by initialize_watering_procedure
                           // ane generate_next_watering_command
                           //
  void ack_node_watering_confirmation(config::Address addr);
  void conclude_watering(HeadStatus resolve_status) {
    this->active_watering_index = std::nullopt;
    this->head_status = resolve_status;
  }
  iNode *get_node(std::size_t node_index) const;
  int no_response_pairing_count = 0;
  void init_pairing_mode();
  void end_pairing_mode() {
    this->head_status = HeadStatus::STANDBY;
    this->no_response_pairing_count = 0;
    Esp_Err_t modify_clock_time(uint8_t hour, uint8_t min);
  }

  std::optional<config::Address> create_node_pending(NodeKey_t key);
  NodeLinkStatus confirm_node_pending(NodeKey_t key, config::Address position);

  void set_node_status(size_t index, NodeStatus status);

  ActionStatus set_node_duration(size_t index,
                                 const NodeTypes::HoseDuration duration);

  ActionStatus set_watering_cycle(size_t index,
                                  const NodeTypes::WateringCycle &cycle);

public:
  Head(iClock &clock, Storage &store, std::function<void()> killNodeTaskFn)
      : clock{clock}, storage{store}, phase_length{clock.get_phase_length()},
        time_pool{phase_length}, kill_node_task_fn{killNodeTaskFn} {};
  std::optional<size_t> get_active_watering_index() const {
    return this->active_watering_index;
  }

  ExtRqManager extRequestsManager{};
  Time::Long get_remaining_time_pool() const { return this->time_pool; }
  HeadStatus &get_head_status() { return this->head_status; };
  const HeadStatus &read_node_status() const { return this->head_status; }
  static constexpr std::size_t max_nodes = config::max_nodes;

  std::optional<config::Address> get_node_addr_by_key(NodeKey_t key);
  bool is_watering_due() const { return this->clock.is_watering_due(); }
  std::optional<UartMessage> next_watering_frame();
  all_node_status_t get_node_statuses();
  std::optional<NodeTypes::DurationSchedule>
  get_node_duration_schedule(size_t node_addr) const;

  NodeStatus get_node_status(size_t addr) {
    if (this->get_node(addr)) {
      return this->get_node(addr)->get_node_status();
    } else {
      return NodeStatus::NODE_NONEXISTANT;
    }
  }
  bool node_exists(size_t addr) const {
    return this->get_node(addr) != nullptr;
  }

  std::optional<NodeTypes::HoseDuration> get_node_hose_duration(size_t index);

  config::Address get_available_address();

  UartMessage create_addressing_frame(NodeKey_t key, config::Address address);
  UartMessage create_watering_frame(config::Address address);

  UartMessage create_status_frame(config::Address address) const;

  int calculate_new_time_pool(size_t index,
                              NodeTypes::HoseDuration new_duration);
  bool is_node_watering_this_phase(size_t addr);

  std::optional<UartMessage> handle_incoming_frame(const UartMessage &msg);
  void process_watering_schedule();
  uint8_t process_external_requests();
  OptMsg process_pairing(OptMsg response, uint32_t tick_key);
  int get_node_retry_count(size_t addr) {
    if (this->get_node(addr)) {
      return this->get_node(addr)->get_retry_count();
    } else {
      return -1;
    }
  }
  int get_no_response_pairing_count() {
    return this->no_response_pairing_count;
  }
  //  all_durations_t retrieve_all_durations();
  void print_node_durations();
  void handle_incoming_node_status(const UartMessage &frame);

  // NOTE: External Requests begin
  void ext_req_pairing_mode() {
    this->extRequestsManager.putRequest(ExtRequest{Req_t::INIT_PAIRING});
  }
  void ext_req_set_node_duration(NodeTypes::HoseDuration duration,
                                 config::Address addr) {
    this->extRequestsManager.putRequest(
        ExtRequest{Req_t::MODIFY_NODE_DURATIONS, {addr, duration}});
  }
  void ext_req_set_node_cycle(uint8_t cycle_bitmask, config::Address addr) {
    this->extRequestsManager.putRequest(
        ExtRequest{Req_t::MODIFY_NODE_CYCLE, {addr, cycle_bitmask}});
  }
  void ext_req_set_clock(uint8_t hour, uint8_t minute) {
    this->extRequestsManager.putRequest(
        ExtRequest{Req_t::MODIFY_CLOCK_TIME, {hour, minute}});
  }

  void ext_req_set_phase(uint8_t hour, uint8_t minute) {
    this->extRequestsManager.putRequest(
        ExtRequest{Req_t::MODIFY_PHASE_START_TIME, {hour, minute}});
  }
  //////////////
  /////////////
};
