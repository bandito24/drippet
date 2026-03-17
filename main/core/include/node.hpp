#pragma once
#include "clock.hpp"
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
enum class NodeStatus {
  INITIALIZING,
  READY,
  IN_QUEUE,
  COMMAND_SENT,
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

  virtual NodeStatus get_node_status() const = 0;
  virtual void set_node_status(NodeStatus status) = 0;
  virtual ActionStatus edit_hose_duration(std::size_t index,
                                          Time::Time_Seconds new_duration) = 0;
  virtual ActionStatus set_node_durations(
      const std::array<Time::Time_Seconds, config::node_hose_count>
          &durations) = 0;
  virtual Time::Time_Seconds get_hose_duration(std::size_t index) const = 0;
  virtual std::array<Time::Time_Seconds, config::node_hose_count>
  get_all_hose_durations() const = 0;
  virtual bool all_durations_zero() const = 0;
  virtual NodeKey_t get_id_key() const = 0;

  virtual uint8_t increase_retry_count() = 0;
  virtual void clear_retry_count() = 0;
};

class Node : public iNode {
public:
  NodeStatus get_node_status() const override;
  void set_node_status(NodeStatus status) override;

  ActionStatus edit_hose_duration(std::size_t index,
                                  Time::Time_Seconds new_duration) override;

  ActionStatus set_node_durations(
      const std::array<Time::Time_Seconds, config::node_hose_count> &durations)
      override;
  bool all_durations_zero() const override;

  Time::Time_Seconds get_hose_duration(std::size_t index) const override;

  Node(NodeKey_t key);

  std::array<Time::Time_Seconds, config::node_hose_count>
  get_all_hose_durations() const override;

  NodeKey_t get_id_key() const override { return id_key; };
  uint8_t increase_retry_count() override {
    this->retry_attempts += 1;
    return this->retry_attempts;
  }
  void clear_retry_count() override { this->retry_attempts = 0; }

private:
  std::array<Time::Time_Seconds, config::node_hose_count> node_hose_durations{};
  NodeKey_t id_key = 0;
  NodeStatus node_status = NodeStatus::INITIALIZING;
  uint8_t retry_attempts = 0;
};

namespace NodeTypes {
using Node = std::unique_ptr<iNode>;
using HoseDurations = std::array<Time::Time_Seconds, config::node_hose_count>;
inline HoseDurations create_durations(std::initializer_list<int> seconds) {
  HoseDurations d{};
  std::size_t i = 0;

  for (Time::Time_Seconds v : seconds) {
    d[i++] = Time::Time_Seconds{v};
  }
  return d;
}
} // namespace NodeTypes
