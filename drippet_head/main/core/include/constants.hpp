
#pragma once
#include "named_types.hpp"
#include <chrono>
#include <stdint.h>
#include <stdio.h>
enum ActionStatus { OK, INVALID_TIME, INVALID_NODE };

using Pin = uint8_t;
using Esp_Err_t = int;
constexpr uint8_t OK_ESP = 0;
using byte_count = int;

namespace Time {
using Time_Point = std::chrono::system_clock::time_point;
using Time_Seconds = uint16_t;
using Long = uint32_t;
constexpr Time_Seconds No_Time = Time_Seconds{0};

constexpr Time_Seconds Day_In_Seconds = static_cast<Time_Seconds>(24 * 60 * 60);
struct UserSetWaterTime {
  uint8_t hour;
  uint8_t minute;
};

} // namespace Time

// enum class Weekdays {
//   SUNDAY,
//   MONDAY,
//   TUESDAY,
//   WEDNESDAY,
//   THURSDAY,
//   FRIDAY,
//   SATURDAY,
// };
enum class CyclePhase {
  FIRST,
  SECOND,
  THIRD,
  FOURTH,
  FIFTH,
  SIXTH,
  SEVENTH,
  CYCLE_LEN
};
inline constexpr int days_in_week = 7;

enum class NodeStatus {
  INITIALIZING,
  READY,
  IN_QUEUE,
  COMMAND_SENT,
  WATERING,
  ERR,
  INVALID_TIME,
  NODE_NONEXISTANT // This value is used for nodes that do not exist yet (not
                   // connected)
};
constexpr size_t PHASE_CYCLE_LEN = static_cast<size_t>(CyclePhase::CYCLE_LEN);
constexpr size_t SELF_NODE_ADDR = 0;
constexpr int DEV_PHASE_LEN = 60;
constexpr int LISTEN_FOR_INIT_COMMS_DUR = 1000;

using NodeKey_t = uint32_t;
