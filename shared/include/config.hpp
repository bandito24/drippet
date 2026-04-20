#include <clock.hpp>
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace config {
constexpr std::size_t max_nodes = 16;
constexpr std::size_t node_hose_count = 5;
using Address = uint8_t;
constexpr uint8_t invalid_address = 0xFF;

constexpr uint16_t MAX_HOSE_DURATION =
    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::days{1})
            .count() /
        (config::max_nodes - 1) -
    300;
// Each hose can at most have an equal fraction of a whole day, 5 min buffer
using HoseDurations = std::array<Time::Time_Seconds, config::node_hose_count>;
} // namespace config
  //
  //
