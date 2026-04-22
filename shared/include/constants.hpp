#include <stdio.h>
#pragma once
#include <chrono>
#include <stdint.h>
enum ActionStatus { OK, INVALID_TIME, INVALID_NODE };

using Pin = uint8_t;
using Esp_Err_t = int;
using byte_count = int;

namespace Time {
using Time_Point = std::chrono::system_clock::time_point;
using Time_Seconds = uint16_t;
constexpr Time_Seconds No_Time = Time_Seconds{0};

constexpr Time_Seconds Day_In_Seconds = static_cast<Time_Seconds>(24 * 60 * 60);
struct WateringSchedule {
  uint8_t hour;
  uint8_t minute;
};

} // namespace Time
