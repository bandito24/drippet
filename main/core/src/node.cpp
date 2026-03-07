#include "constants.hpp"
#include "head.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <config.hpp>
#include <cstddef>
#include <node.hpp>
#include <time.hpp>

Time::Time_Point Node::next_water_timepoint() const {
  return non_watering_interval_begin + watering_interval;
}

NodeStatus Node::get_node_status() const { return this->node_status; }
void Node::send_node_directions() const {
  // TODO: Hardware level transmit;
}
void Node::set_node_status(NodeStatus status) { this->node_status = status; }
ActionStatus Node::edit_hose_duration(std::size_t index,
                                      Time::Time_Seconds new_duration) {
  assert(index < config::node_hose_count);
  Time::Time_Seconds duration_check =
      calculate_cumulative_watering(node_hose_durations) -
      node_hose_durations[index] + new_duration;

  if (duration_check >= watering_interval) {
    return ActionStatus::INVALID_TIME;
  }
  node_hose_durations[index] = new_duration;
  return ActionStatus::OK;
}

ActionStatus Node::configure_watering_schedule(
    Time::Time_Seconds watering_interval,
    const std::array<Time::Time_Seconds, config::node_hose_count>
        &hose_durations) {
  return ActionStatus::OK;
  // this->watering_interval = watering_interval;
  // Time::Time_Seconds water_duration =
  //     calculate_cumulative_watering(hose_durations);
  // if (water_duration >= watering_interval) {
  //   return ActionStatus::INVALID_TIME;
  // }

  // std::copy(hose_durations.begin(), hose_durations.end(),
  //           this->node_hose_durations.begin());
  // return set_watering_interval(watering_interval);
}
config::Address Node::get_address() const { return hardware_address; }

Node::Node(
    Time::Time_Seconds wateringInterval,
    std::array<Time::Time_Seconds, config::node_hose_count> hose_durations,
    config::Address hardwareAddress)
    : hardware_address{hardwareAddress}, watering_interval{wateringInterval} {

  Time::Time_Seconds water_duration =
      calculate_cumulative_watering(hose_durations);
  if (water_duration >= this->watering_interval) {
    node_status = NodeStatus::ERR_DURATION;
  } else {
    node_status = NodeStatus::INITIALIZING;
  }

  std::copy(hose_durations.begin(), hose_durations.end(),
            node_hose_durations.begin());
};
// Node::Node(config::Address hardwareAddress)
//     : hardware_address(hardwareAddress),
//     node_status(NodeStatus::INITIALIZING) {
//   std::fill(node_hose_durations.begin(), node_hose_durations.end(),
//             Time::Time_Seconds{0});
// }
//  Node::Node() : node_status(NodeStatus::INACTIVE) {
//
//    std::fill(node_hose_durations.begin(), node_hose_durations.end(),
//              Time::Time_Seconds{0});
//  };

Time::Time_Seconds Node::calculate_cumulative_watering(
    std::array<Time::Time_Seconds, config::node_hose_count> durations) const {

  Time::Time_Seconds cumulative = Time::Time_Seconds{0};
  for (Time::Time_Seconds seconds : durations) {
    cumulative += seconds;
  }
  return cumulative;
}

ActionStatus Node::set_node_durations(

    const std::array<Time::Time_Seconds, config::node_hose_count> &durations) {

  Time::Time_Seconds cumulative =
      calculate_cumulative_watering(node_hose_durations);
  if (cumulative >= watering_interval) {
    return ActionStatus::INVALID_TIME;
  }
  std::copy(durations.begin(), durations.end(), node_hose_durations.begin());
  return ActionStatus::OK;
}

ActionStatus Node::set_watering_interval(Time::Time_Seconds interval) {
  Time::Time_Seconds cumulative =
      calculate_cumulative_watering(node_hose_durations);
  if (cumulative >= interval) {
    return ActionStatus::INVALID_TIME;
  }
  return ActionStatus::OK;
}
NodeTypes::HoseDurations Node::get_all_hose_durations() const {
  return node_hose_durations;
}
Time::Time_Seconds Node::get_watering_interval() const {
  return watering_interval;
}

Time::Time_Seconds Node::get_hose_duration(std::size_t index) const {
  assert(index < config::node_hose_count);
  return node_hose_durations[index];
}
Node::Node(NodeTypes::HoseDurations hose_durations) {

  Time::Time_Seconds water_duration =
      calculate_cumulative_watering(hose_durations);
  if (water_duration >= Time::Day_In_Seconds) {
    node_status = NodeStatus::ERR_DURATION;
  } else {
    node_status = NodeStatus::INITIALIZING;
  }

  std::copy(hose_durations.begin(), hose_durations.end(),
            node_hose_durations.begin());
}
Node::Node(config::Address index_address, NodeKey_t key)
    : hardware_address(index_address), id_key(key),
      node_status(NodeStatus::INITIALIZING) {}
