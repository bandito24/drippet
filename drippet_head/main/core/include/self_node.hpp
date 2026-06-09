#pragma once
#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "protocol.hpp"
#include "steady_clock.hpp"
#include "switch.hpp"
#include <array>
#include <cstdint>
#include <optional>

enum class RC { OK, ERR };
using OptMsg = std::optional<UartMessage>;
using SelfHoseDurations = std::array<Time::Long, config::node_hose_count>;

// seed with uint32_t xTaskGetTickCount()
class SelfNode {
public:
  SelfNode(SteadyClock &clk, SolenoidManager &solenoidManager)
      : clock{clk}, solenoid_manager{solenoidManager} {};
  Esp_Err_t init();
  std::optional<UartMessage> handle_incoming_frame(UartMessage &msg);
  const NodeStatus &get_status() const { return this->status; }
  void process_watering_schedule();
  NodeKey_t get_key() const { return this->self_key; };
  size_t get_active_hose_index() const { return this->active_hose_index; }
  config::Address get_addr() const { return this->self_addr; }
  const SelfHoseDurations &get_hose_durations() const {
    return this->hose_durations;
  }

private:
  SteadyClock &clock;
  SolenoidManager &solenoid_manager;
  config::Address self_addr = ADDR_UNSET;
  NodeKey_t self_key = 0;
  SelfHoseDurations hose_durations{};
  size_t active_hose_index = HOSE_INACTIVE_IDX;
  void change_active_hose(size_t hose_index);

  std::optional<UartMessage>
  initialize_watering(Protocol::FrameDataArray &durations);
  bool has_addr() const { return this->self_addr != ADDR_UNSET; }
  Time::Long last_evt_time{};
  void update_last_evt_time() { this->last_evt_time = this->clock.now(); }
  void conclude_watering();
  NodeStatus status = NodeStatus::INITIALIZING;
  void complete_initialization();
  std::optional<UartMessage> handle_unsynced_message(UartMessage &msg);
  // Just little endian key for convenient with uart mesages
  std::array<uint16_t, 2> serial_key{};

  bool deactivate = false;
};
