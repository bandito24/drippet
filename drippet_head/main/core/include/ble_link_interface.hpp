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
};
constexpr size_t CMD_IDX = 0;

using FlatNodeSchedule = std::array<uint8_t, 3>;

class BLELinkInterface {
public:
  BLELinkInterface(Head &head) : head_node(head) {};
  void handle_writes(std::span<const uint8_t> pkt);
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
