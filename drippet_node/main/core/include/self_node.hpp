#include "clock.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "protocol.hpp"
#include <array>
#include <cstdint>
#include <optional>
// seed with uint32_t xTaskGetTickCount()
class SelfNode {
public:
  SelfNode(Driver &driver, Clock &clock, uint32_t randomKeySeed)
      : uart(driver) {
    this->self_key = static_cast<uint16_t>(randomKeySeed);
  };
  Esp_Err_t init();
  std::optional<UartMessage> handle_incoming_frame(UartMessage &msg);
  UartMessage generate_discovery_message();

private:
  Driver &uart;
  size_t self_addr = ADDR_UNSET;
  uint16_t self_key;
  std::array<Time::Long, config::node_hose_count> hose_intervals{};
  size_t current_node_index =
      config::node_hose_count; // means that its not on any hose (closed)
  void initialize_watering(UartMessage &msg);
};
