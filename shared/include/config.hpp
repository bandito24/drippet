#pragma once
#include "constants.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace config {
constexpr std::size_t max_nodes = 12;
constexpr std::size_t node_hose_count = 5;
using Address = uint8_t;
constexpr uint8_t invalid_address = 0xFF;

constexpr uint32_t MAX_WATERING_SECONDS =
    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::days{1})
        .count() -
    (5 * 60); //% minute buffer for communication retries
constexpr uint16_t MAX_HOSE_DURATION =
    MAX_WATERING_SECONDS / (config::max_nodes - 1) - 300;
// 86400 seconds in a day
// Each hose can at most have an equal fraction of a whole day, 5 min buffer
// (16980 seconds for five hoses, 283 mins, 4.71 hours)
// (5460 seconds for 15 hoses, 96 mins, 1.5 hours)
// (2880 seconds for 30 hoses, 48 mins)
// (1440 seconds for 60 hoses, 24 mins)
using HoseDurations = std::array<Time::Time_Seconds, config::node_hose_count>;
} // namespace config
  //
  //

constexpr size_t HOSE_INACTIVE_IDX = config::node_hose_count;
