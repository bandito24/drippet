#include "driver.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include <assert.h>

#include "config.hpp"
#include "node.hpp"
#include "protocol.hpp"
namespace Mocks {

constexpr NodeKey_t sample_key = 500;
constexpr NodeTypes::HoseDurations hose_durations = {1, 2, 3, 4, 5};
inline NodeTypes::HoseDurations get_new_hose_durations(int increaser) {
  NodeTypes::HoseDurations ret{};
  for (size_t i = 0; i < hose_durations.size(); i++) {
    ret[i] = hose_durations[i] * (1 + increaser);
  }
  return ret;
}
inline UartMessage incoming_adressing_frame(config::Address addr,
                                            NodeKey_t key = sample_key) {
  return {.address = addr,
          .command = Protocol::Command::ADDRESSING,
          .data = Protocol::FrameDataArray{key},
          .data_length = 1};
}

inline UartMessage incoming_discovery_frame(NodeKey_t key = sample_key) {
  return {.address = config::max_nodes,
          .command = Protocol::Command::DISCOVERY,
          .data = Protocol::FrameDataArray{key},
          .data_length = 1};
}
inline UartMessage incoming_ack_frame(config::Address addr) {
  return {.address = addr, .command = Protocol::Command::ACK, .data_length = 0};
}
inline void populate_head_nodes(Head &head,
                                size_t node_count = config::max_nodes) {

  assert(node_count <= config::max_nodes);
  for (size_t addr = 0; addr < node_count; addr++) {
    head.create_node_pending(addr);
    head.confirm_node_pending(addr, addr);
    auto new_durations = get_new_hose_durations(addr);
    head.set_node_durations(addr, new_durations);
  }
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
          6, 0, 7, 0, 8, 0, 9, 0, 10, 1};
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
