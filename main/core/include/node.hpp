#pragma once
#include <array>
#include <config.hpp>
#include <constants.hpp>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <time.hpp>

enum HardwareStatus {
  ERR_NONE,
};
enum class NodeStatus {
  IDLE,
  WATERING,
  READY,
  COMMAND_SENT,
  INITIALIZING,
  INACTIVE,
  ERR,
  ERR_DURATION,
  UNRESPONSIVE
};
enum Status { IS_GOOD };
struct iValve {
  virtual HardwareStatus open_valve() = 0;
  virtual HardwareStatus close_valve() = 0;
};

using NodeKey_t = uint16_t;

// Todo: Complete these implementations
class MainValve : public iValve {
  HardwareStatus open_valve() override { return HardwareStatus::ERR_NONE; };
  HardwareStatus close_valve() override { return HardwareStatus::ERR_NONE; };
};
struct NodeDirections {
  Time::Time_Seconds node_hose_durations[config::node_hose_count];
};

struct iNode {
  virtual ~iNode() = default;

  virtual Time::Time_Point next_water_timepoint() const = 0;
  virtual void send_node_directions() const = 0;
  virtual NodeStatus get_node_status() const = 0;
  virtual void set_node_status(NodeStatus status) = 0;

  virtual Time::Time_Seconds calculate_cumulative_watering(
      std::array<Time::Time_Seconds, config::node_hose_count> durations)
      const = 0;

  virtual ActionStatus edit_hose_duration(std::size_t index,
                                          Time::Time_Seconds new_duration) = 0;
  virtual ActionStatus set_watering_interval(Time::Time_Seconds interval) = 0;

  virtual ActionStatus set_node_durations(
      const std::array<Time::Time_Seconds, config::node_hose_count>
          &durations) = 0;

  virtual Time::Time_Seconds get_hose_duration(std::size_t index) const = 0;

  virtual ActionStatus configure_watering_schedule(
      Time::Time_Seconds watering_interval,
      const std::array<Time::Time_Seconds, config::node_hose_count>
          &hose_durations) = 0;

  virtual config::Address get_address() const = 0;
  virtual Time::Time_Seconds get_watering_interval() const = 0;
  virtual std::array<Time::Time_Seconds, config::node_hose_count>
  get_all_hose_durations() const = 0;
  virtual NodeKey_t get_id_key() const = 0;
};

class Node : public iNode {
public:
  Time::Time_Point next_water_timepoint() const override;
  void send_node_directions() const override;
  NodeStatus get_node_status() const override;
  void set_node_status(NodeStatus status) override;

  Time::Time_Seconds calculate_cumulative_watering(
      std::array<Time::Time_Seconds, config::node_hose_count> durations)
      const override;

  ActionStatus edit_hose_duration(std::size_t index,
                                  Time::Time_Seconds new_duration) override;

  ActionStatus set_node_durations(
      const std::array<Time::Time_Seconds, config::node_hose_count> &durations)
      override;

  Time::Time_Seconds get_hose_duration(std::size_t index) const override;
  ActionStatus set_watering_interval(Time::Time_Seconds interval) override;

  ActionStatus configure_watering_schedule(
      Time::Time_Seconds watering_interval,
      const std::array<Time::Time_Seconds, config::node_hose_count>
          &hose_durations) override;

  Node(Time::Time_Seconds watering_interval,
       std::array<Time::Time_Seconds, config::node_hose_count> hose_durations,
       config::Address hardwareAddress);

  Node(config::Address index_address, NodeKey_t key);
  Node(std::array<Time::Time_Seconds, config::node_hose_count> hose_durations);

  config::Address get_address() const override;

  std::array<Time::Time_Seconds, config::node_hose_count>
  get_all_hose_durations() const override;

  Time::Time_Seconds get_watering_interval() const override;
  NodeKey_t get_id_key() const override { return id_key; };

private:
  config::Address hardware_address = config::invalid_address;
  // Time::Time_Seconds node_hose_durations[config::node_hose_count] = {
  //   Time::No_Time};
  std::array<Time::Time_Seconds, config::node_hose_count> node_hose_durations{};
  Time::Time_Seconds watering_interval = Time::Day_In_Seconds;
  Time::Time_Point non_watering_interval_begin;
  NodeKey_t id_key = 0;

  NodeStatus node_status = NodeStatus::INITIALIZING;
};

namespace NodeTypes {
using Node_Link = std::unique_ptr<iNode>;
using HoseDurations = std::array<Time::Time_Seconds, config::node_hose_count>;
inline HoseDurations create_durations(std::initializer_list<int> seconds) {
  HoseDurations d{};
  std::size_t i = 0;

  for (int v : seconds) {
    d[i++] = Time::Time_Seconds{v};
  }
  return d;
}
} // namespace NodeTypes
