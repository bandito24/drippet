#include "clock.hpp"
#include "constants.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <config.hpp>
#include <cstddef>
#include <node.hpp>

NodeStatus Node::get_node_status() const { return this->node_status; }
void Node::set_node_status(NodeStatus status) { this->node_status = status; }
ActionStatus Node::edit_hose_duration(std::size_t index,
                                      Time::Time_Seconds new_duration) {
  assert(index < config::node_hose_count);
  if (new_duration > config::MAX_HOSE_DURATION) {
    return ActionStatus::INVALID_TIME;
  }
  node_hose_durations[index] = new_duration;
  return ActionStatus::OK;
}

ActionStatus Node::set_node_durations(

    const std::array<Time::Time_Seconds, config::node_hose_count> &durations) {
  for (Time::Time_Seconds duration : durations) {
    if (duration > config::MAX_HOSE_DURATION) {
      return ActionStatus::INVALID_TIME;
    }
  }

  std::copy(durations.begin(), durations.end(), node_hose_durations.begin());
  return ActionStatus::OK;
}

NodeTypes::HoseDurations Node::get_all_hose_durations() const {
  return node_hose_durations;
}
bool Node::all_durations_zero() const {
  for (Time::Time_Seconds duration : this->node_hose_durations) {
    if (duration != Time::Time_Seconds{0}) {
      return false;
    }
  }
  return true;
}

Time::Time_Seconds Node::get_hose_duration(std::size_t index) const {
  assert(index < config::node_hose_count);
  return node_hose_durations[index];
}
Node::Node(NodeKey_t key)
    : id_key(key), node_status(NodeStatus::INITIALIZING) {}
