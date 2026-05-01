#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "protocol.hpp"
#include "steady_clock.hpp"
#include <array>
#include <cstdint>
#include <optional>

enum class RC { OK, ERR };

// seed with uint32_t xTaskGetTickCount()
class SelfNode {
public:
  SelfNode(Driver &driver, Clock &clk) : uart{driver}, clock{clk} {
    this->update_last_evt_time();
  };
  Esp_Err_t init();
  std::optional<UartMessage> handle_incoming_frame(UartMessage &msg);
  UartMessage generate_discovery_message();
  NodeStatus get_status() const { return this->status; }
  void process_watering_schedule();

private:
  Driver &uart;
  Clock &clock;
  config::Address self_addr = ADDR_UNSET;
  NodeKey_t self_key = 0;
  std::array<Time::Long, config::node_hose_count> hose_durations{};
  size_t active_hose_index =
      config::node_hose_count; // means that its not on any hose (closed)
  void change_active_hose(size_t hose_index);

  void initialize_watering(UartMessage &msg);
  bool has_addr() const { return this->self_addr != ADDR_UNSET; }
  Time::Long last_evt_time{};
  void update_last_evt_time() { this->last_evt_time = this->clock.now(); }
  void initialize_watering(Protocol::FrameDataArray &durations);
  void conclude_watering();
  NodeStatus status = NodeStatus::INITIALIZING;
  void complete_initialization();
  std::optional<UartMessage> handle_unsynced_message(UartMessage &msg);
  // Just little endian key for convenient with uart mesages
  std::array<uint16_t, 2> serial_key{};

  bool deactivate = false;
};
