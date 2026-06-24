#pragma once
#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "protocol.hpp"
#include "steady_clock.hpp"
#include "switch.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>

enum class RC { OK, ERR };
using SolenoidPtr = std::unique_ptr<SolenoidValve>;
using SwitchPtr = std::unique_ptr<Switch>;

// seed with uint32_t xTaskGetTickCount()
class SelfNode {
public:
  SelfNode(SteadyClock &clk, SolenoidPtr _solenoid, SwitchPtr _downstreamPower)
      : clock{clk}, solenoid{std::move(_solenoid)},
        downstreamPower{std::move(_downstreamPower)} {};

  ~SelfNode() {
    this->downstreamPower->disable();
    this->solenoid->disable();
  }
  Esp_Err_t init();
  std::optional<UartMessage> handle_incoming_frame(UartMessage &msg);
  const NodeStatus &get_status() const { return this->status; }
  void process_watering_schedule();
  NodeKey_t get_key() const { return this->self_key; };
  config::Address get_addr() const { return this->self_addr; }
  void activate_hose();
  Time::Long get_hose_duration() const { return this->hose_duration; }
  bool is_watering() const { return this->solenoid->is_enabled(); }
  bool is_downstream_enabled() { return this->downstreamPower->is_enabled(); }
  bool is_solenoid_enabled() { return this->solenoid->is_enabled(); }

private:
  SteadyClock &clock;
  SolenoidPtr solenoid;
  SwitchPtr downstreamPower;
  config::Address self_addr = ADDR_UNSET;
  NodeKey_t self_key = 0;
  Time::Long hose_duration{};

  UartMessage initialize_watering(Time::Long duration);
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
