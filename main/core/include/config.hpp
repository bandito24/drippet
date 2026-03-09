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
using HoseDurations = std::array<Time::Time_Seconds, config::node_hose_count>;
} // namespace config
  //
  //
