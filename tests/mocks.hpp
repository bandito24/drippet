#pragma once
#include "constants.hpp"
#include "driver.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "switch.hpp"
#include "util.hpp"
#include <assert.h>
#include <optional>

#include "config.hpp"
#include "node.hpp"
#include "protocol.hpp"
namespace Mocks {

constexpr NodeKey_t sample_key = 500;
constexpr NodeTypes::HoseDuration hose_duration = {100};
inline NodeTypes::HoseDuration get_new_hose_durations(int increaser) {
  return hose_duration * (1 + increaser);
}
inline UartMessage incoming_adressing_frame(config::Address addr,
                                            NodeKey_t key = sample_key) {
  auto key_arr = Util::serialize_key(key);
  return {.address = addr,
          .command = Protocol::Command::ADDRESSING,
          .data = Protocol::FrameDataArray{key_arr[0], key_arr[1]},
          .data_length = 2};
}

inline UartMessage incoming_discovery_frame(NodeKey_t key = sample_key) {

  auto key_arr = Util::serialize_key(key);
  return {.address = ADDR_UNSET,
          .command = Protocol::Command::DISCOVERY,
          .data = Protocol::FrameDataArray{key_arr[0], key_arr[1]},
          .data_length = 2};
}
inline UartMessage incoming_ack_frame(config::Address addr) {
  return {.address = addr, .command = Protocol::Command::ACK, .data_length = 0};
}

inline UartMessage incoming_status_frame(config::Address addr,
                                         NodeStatus status) {
  return {.address = addr,
          .command = Protocol::Command::STATUS,
          .data = {static_cast<uint16_t>(status)},
          .data_length = 1};
}
inline void populate_head_nodes(Head &head,
                                size_t node_count = config::max_nodes) {

  assert(node_count <= config::max_nodes);
  for (size_t addr = 0; addr < node_count; addr++) {
    head.create_node_pending(addr);
    head.confirm_node_pending(addr, addr);
    auto new_durations = get_new_hose_durations(addr);
    head.set_node_duration(addr, new_durations);
  }
}
inline void populate_single_node(Head &head, config::Address addr,
                                 NodeTypes::HoseDuration duration) {
  auto key = NodeKey_t{addr};
  assert(head.get_node_by_key(key) == std::nullopt);
  head.create_node_pending(addr);
  head.confirm_node_pending(key, addr);
  head.set_node_duration(addr, duration);
}

inline uint8_t uint8(BLE::Cmds cmd) { return static_cast<uint8_t>(cmd); }

} // namespace Mocks
  //

namespace BleMocks {
inline std::array<uint8_t, 13> pkt_write_row() {
  return {Mocks::uint8(BLE::Cmds::WRITE_ROW), // COMMAND
          11,                                 // DATA_LEN
          0,                                  // ROW INDEX
          // durations (little-endian)
          6, 0};
}

inline std::array<uint8_t, 3> pkt_load_row() {
  return {
      Mocks::uint8(BLE::Cmds::LOAD_ROW), // COMMAND
      1,                                 // DATA_LEN
      0                                  // ROW
  };
}
inline std::array<uint8_t, 6> pkt_write_cell() {
  return {
      Mocks::uint8(BLE::Cmds::WRITE_CELL), // COMMAND
      4,                                   // DATA_LEN
      0,                                   // ROW
      0,                                   // COLUMN

      235,
      0 // (little-endian)
        // }
  };
}
} // namespace BleMocks
class SolenoidMock : public SolenoidValve {
public:
  Esp_Err_t enable() override {
    enabled = true;
    return 0;
  };
  Esp_Err_t init() override { return 0; };
  Esp_Err_t disable() override {
    enabled = false;
    return 0;
  };
  bool is_enabled() const override { return enabled; };

private:
  bool enabled = false;
};
