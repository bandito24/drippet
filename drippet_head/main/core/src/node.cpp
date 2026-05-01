#include "clock.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "util.hpp"
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
      this->node_status = NodeStatus::INVALID_TIME;
      return ActionStatus::INVALID_TIME;
    }
  }

  node_hose_durations = durations; // simpler than std::copy
  return ActionStatus::OK;
}

ActionStatus
Node::set_node_durations(const NodeTypes::DurationSchedule &dur_sch) {
  ActionStatus rc = this->set_node_durations(dur_sch.durations);
  if (rc != ActionStatus::OK) {
    return rc;
  }

  Logger::log_simple("THID IS THING");
  Util::print_schedule(dur_sch.schedule);
  this->weekly_waterings = dur_sch.schedule;
  this->node_hose_durations = dur_sch.durations;

  Logger::log_simple("THID IS THING 2");
  Util::print_schedule(this->weekly_waterings);
  return ActionStatus::OK;
};

const NodeTypes::HoseDurations &Node::get_all_hose_durations() const {
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
void Node::print_hose_durations(size_t addr) {
  printf("Node %zu | ", addr);
  for (size_t hose = 0; hose < config::node_hose_count; ++hose) {
    printf("%5u ", static_cast<unsigned>(this->node_hose_durations[hose]));
  }
  printf("\n");

  printf("NSCH %2zu | ", addr);
  for (size_t day = 0; day < days_in_week; ++day) {
    printf("%5s ", this->weekly_waterings[day] ? "true" : "false");
  }
  printf("\n");
}
