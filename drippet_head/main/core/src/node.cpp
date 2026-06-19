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

ActionStatus Node::set_node_duration(Time::Time_Seconds duration) {
  node_hose_duration = duration; // simpler than std::copy
  return ActionStatus::OK;
}
NodeTypes::DurationSchedule Node::get_duration_schedule() const {
  return {this->node_hose_duration, this->water_cycle};
}

ActionStatus
Node::set_node_durations(const NodeTypes::DurationSchedule &dur_sch) {
  ActionStatus rc = this->set_node_duration(dur_sch.duration);
  if (rc != ActionStatus::OK) {
    return rc;
  }

  this->water_cycle = dur_sch.cycle;
  this->node_hose_duration = dur_sch.duration;

  return ActionStatus::OK;
};

NodeTypes::HoseDuration Node::get_hose_duration() const {
  return node_hose_duration;
}

Node::Node(NodeKey_t key)
    : id_key(key), node_status(NodeStatus::INITIALIZING) {}
void Node::print_hose_duration(size_t addr) const {
  printf("Node %zu | ", addr);
  printf("%5u ", static_cast<unsigned>(this->node_hose_duration));
  printf("\n");

  printf("NSCH %2zu | ", addr);
  for (size_t day = 0; day < days_in_week; ++day) {
    printf("%5s ", this->water_cycle[day] ? "true" : "false");
  }
  printf("\n");
}
