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

class BLELinkInterface {
public:
  BLELinkInterface(Head &head) : head_node(head){};
  void handle_read_buffer();
  void handle_write_buffer();
  void load_incoming_buffer(std::span<const uint8_t> pkt);
  const SerializedPacketBuffer &read_outgoing_buffer() const;

private:
  void reset_buffer(SerializedPacketBuffer &buffer) {
    buffer = SerializedPacketBuffer{};
  }
  Head &head_node;
  SerializedPacketBuffer incoming_buffer{};
  SerializedPacketBuffer outgoing_buffer{};
  void load_outgoing_buffer(std::span<const uint8_t> pkt);
  const SerializedPacketBuffer &read_incoming_buffer() const;
};
