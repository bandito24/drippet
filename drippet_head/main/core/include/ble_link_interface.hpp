#pragma once
#include "ble_types.hpp"
#include "head.hpp"
#include <array>
#include <span>
#include <stdint.h>

using DataPkt = std::array<uint8_t, BLE::MTU_SIZE>;
struct SerializedPacketBuffer {
  DataPkt data;
  size_t len;
  bool operator==(const SerializedPacketBuffer &other) const = default;
};
constexpr size_t CMD_IDX = 0;

using FlatNodeSchedule = std::array<uint8_t, 3>;

class BLELinkInterface {
public:
  BLELinkInterface(Head &head) : head_node(head) {};
  void handle_writes(std::span<const uint8_t> pkt);

  // READ_CONFIGURATION layout (BLE::READ_CONF_PKT_LEN = 13 bytes, multi-byte
  // fields little-endian). Index constants live in ble_types.hpp.
  //   [READ_CONF_TS_IDX]         0-7  Head::get_curr_time(false) as time_t
  //                                   (int64_t UTC Unix seconds, offset NOT
  //                                   applied)
  //   [READ_CONF_OFFSET_IDX]     8-9  iClock::UTC_OFFSET_MIN (int16_t minutes,
  //                                   Time::OFFSET_RANGES). local = utc + offset
  //   [READ_CONF_WATER_SET_IDX]  10   1 if Head::get_hourmin_next_phase() has a
  //                                   value, else 0
  //   [READ_CONF_WATER_HOUR_IDX] 11   HourMin::hour, local (0-23). 0 if unset
  //   [READ_CONF_WATER_MIN_IDX]  12   HourMin::minute, local (0-59). 0 if unset
  const SerializedPacketBuffer &handle_reads(BLE::Read_T read);

  static FlatNodeSchedule
  duration_schedule_to_bytes(const NodeTypes::DurationSchedule &dur_sch);

  static NodeTypes::DurationSchedule
  bytes_to_duration_schedule(const FlatNodeSchedule &byte_sch);

private:
  void reset_buffer(SerializedPacketBuffer &buffer) {
    buffer = SerializedPacketBuffer{};
  }
  Head &head_node;
  SerializedPacketBuffer buffer{};
  void load_outgoing_buffer(std::span<const uint8_t> pkt);
  const SerializedPacketBuffer &read_incoming_buffer() const;
};
