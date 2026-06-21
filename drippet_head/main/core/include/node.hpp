#pragma once
#include "clock.hpp"
#include "constants.hpp"
#include <array>
#include <chrono>
#include <config.hpp>
#include <constants.hpp>
#include <cstdint>
#include <initializer_list>
#include <memory>

enum HardwareStatus {
  ERR_NONE,
};

namespace NodeTypes {
using HoseDuration = Time::Time_Seconds;
// using WateringSchedule = std::array<bool, PHASE_CYCLE_LEN>;
//
using WateringCycle = std::array<bool, PHASE_CYCLE_LEN>;
struct DurationSchedule {
  NodeTypes::HoseDuration duration{};
  NodeTypes::WateringCycle cycle{true, true, true, true, true, true, true};

  bool operator==(DurationSchedule &other) {
    return other.duration == this->duration && other.cycle == this->cycle;
  }
};
} // namespace NodeTypes

enum Status { IS_GOOD };
struct iValve {
  virtual HardwareStatus open_valve() = 0;
  virtual HardwareStatus close_valve() = 0;
};

// Todo: Complete these implementations
class MainValve : public iValve {
  HardwareStatus open_valve() override { return HardwareStatus::ERR_NONE; };
  HardwareStatus close_valve() override { return HardwareStatus::ERR_NONE; };
};

struct iNode {
  virtual ~iNode() = default;

  virtual NodeStatus get_node_status() const = 0;
  virtual void set_node_status(NodeStatus status) = 0;
  virtual ActionStatus set_node_duration(NodeTypes::HoseDuration duration) = 0;

  virtual ActionStatus
  set_node_durations(const NodeTypes::DurationSchedule &dur_sch) = 0;

  virtual Time::Time_Seconds get_hose_duration() const = 0;
  virtual NodeKey_t get_id_key() const = 0;
  virtual uint8_t increase_retry_count() = 0;
  virtual void clear_retry_count() = 0;
  virtual uint8_t get_retry_count() = 0;
  // virtual std::array<bool, PHASE_CYCLE_LEN> &get_watering_cycle() = 0;
  // virtual void
  // set_watering_cycle(const NodeTypes::WateringCycle &new_water) = 0;

  virtual std::array<bool, PHASE_CYCLE_LEN> &get_watering_cycle() = 0;
  virtual void
  set_watering_cycle(const NodeTypes::WateringCycle &new_water) = 0;
  virtual void print_hose_duration(size_t addr) const = 0;
  virtual NodeTypes::DurationSchedule get_duration_schedule() const = 0;
};

class Node : public iNode {
public:
  NodeStatus get_node_status() const override;
  void set_node_status(NodeStatus status) override;

  ActionStatus set_node_duration(NodeTypes::HoseDuration duration) override;

  ActionStatus
  set_node_durations(const NodeTypes::DurationSchedule &dur_sch) override;

  NodeTypes::DurationSchedule get_duration_schedule() const override;

  Time::Time_Seconds get_hose_duration() const override;

  Node(NodeKey_t key);

  std::array<bool, PHASE_CYCLE_LEN> &get_watering_cycle() override {
    return this->water_cycle;
  };
  void set_watering_cycle(const NodeTypes::WateringCycle &new_water) override {
    this->water_cycle = new_water;
  }

  NodeKey_t get_id_key() const override { return id_key; };
  uint8_t increase_retry_count() override {
    this->retry_attempts += 1;
    return this->retry_attempts;
  }
  void clear_retry_count() override { this->retry_attempts = 0; }
  uint8_t get_retry_count() override { return this->retry_attempts; }

  Time::Time_Seconds node_hose_duration{};

  void print_hose_duration(size_t addr) const override;

private:
  std::array<bool, PHASE_CYCLE_LEN> water_cycle{true, true, true, true,
                                                true, true, true};

  NodeKey_t id_key = 0;
  NodeStatus node_status = NodeStatus::INITIALIZING;
  uint8_t retry_attempts = 0;
};

namespace NodeTypes {
using Node = std::unique_ptr<iNode>;
} // namespace NodeTypes
