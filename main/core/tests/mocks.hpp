#include "head.hpp"

#include "config.hpp"
#include "node.hpp"
#include "protocol.hpp"
namespace Mocks {

constexpr NodeKey_t sample_key = 500;
constexpr NodeTypes::HoseDurations hose_durations = {1, 2, 3, 4, 5};
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
inline void populate_head_nodes(Head &head) {

  for (size_t addr = 0; addr < config::max_nodes; addr++) {
    head.create_node_pending(addr);
    head.confirm_node_pending(addr, addr);
    head.get_node(addr)->set_node_durations(hose_durations);
  }
}
} // namespace Mocks
