#pragma once
#include "config.hpp"
#include "constants.hpp"
#include "head.hpp"
#include "node.hpp"
#include "protocol_types.hpp"
#include <span>

using FlatSchedule = std::array<uint8_t, 4>;

class GattAttribute {
private:
  Head &head;
  BLE::Status validate_packet(std::span<uint8_t> pkt);
  // Two duration bytes for uint16_t then a bitmask for cycle

public:
  static FlatSchedule
  duration_schedule_to_bytes(const NodeTypes::DurationSchedule &dur_sch,
                             config::Address address);

  static NodeTypes::DurationSchedule
  bytes_to_duration_schedule(const FlatSchedule &byte_sch);
  GattAttribute(Head &headNode) : head{headNode} {};
  BLE::Status load_duration_buffer(size_t addr);
  FlatSchedule duration_buffer{};
  // An addr specifier, 2 uint16_t to uint8_t, and a bitmask for water cycle

  BLE::Status handle_incoming_write(std::span<uint8_t> raw_data);
  uint16_t duration_chr_handle;
};
