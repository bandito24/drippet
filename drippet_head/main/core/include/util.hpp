#pragma once
#include "constants.hpp"
#include "node.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <span>
namespace Util {
inline void put_le16(void *buf, uint16_t x) {

  auto u8ptr = static_cast<uint8_t *>(buf);

  u8ptr[0] = (uint8_t)x;
  u8ptr[1] = (uint8_t)(x >> 8);
}

inline void put_le32(void *buf, uint32_t x) {

  auto u8ptr = static_cast<uint8_t *>(buf);
  u8ptr[0] = (uint8_t)(x);
  u8ptr[1] = (uint8_t)(x >> 8);
  u8ptr[2] = (uint8_t)(x >> 16);
  u8ptr[3] = (uint8_t)(x >> 24);
}

inline uint16_t get_le16(const void *buf) {

  uint16_t x;
  auto u8ptr = static_cast<const uint8_t *>(buf);

  x = u8ptr[0];
  x |= (uint16_t)u8ptr[1] << 8;

  return x;
}
inline NodeTypes::WateringCycle byte_to_water_cycle(uint8_t bitmask) {
  NodeTypes::WateringCycle res{};
  for (size_t i = 0; i < res.size(); i++) {
    res[i] = (bitmask & (1U << i)) != 0;
  }
  return res;
}

inline uint8_t water_cycle_to_bytes(NodeTypes::WateringCycle cycle) {
  uint8_t bitmask{};
  for (size_t i = 0; i < cycle.size(); i++) {
    bitmask |= static_cast<uint8_t>(cycle.at(i)) << i;
  }
  return bitmask;
}

inline std::array<uint16_t, 2> serialize_key(NodeKey_t key) {
  std::array<uint16_t, 2> res{};
  res[0] = static_cast<uint16_t>(key);
  res[1] = static_cast<uint16_t>(key >> 16);
  return res;
}
inline NodeKey_t deserialize_key(std::span<const uint16_t> input) {
  return (uint32_t)input[0] | ((uint32_t)input[1] << 16);
}

inline uint32_t get_le32(const void *buf) {

  uint32_t x;
  auto u8ptr = static_cast<const uint8_t *>(buf);

  x = u8ptr[0];
  x |= (uint32_t)u8ptr[1] << 8;
  x |= (uint32_t)u8ptr[2] << 16;
  x |= (uint32_t)u8ptr[3] << 24;

  return x;
}
inline void le16_to_le8(std::span<uint8_t> out, std::span<uint16_t> in) {
  assert(out.size() >= (in.size() * 2));

  size_t insert_idx = 0;
  for (size_t i = 0; i < in.size(); i++) {
    Util::put_le16(&out.data()[insert_idx], in[i]);
    insert_idx += 2;
  }
}

inline void le32_to_le8(std::span<uint8_t> out, std::span<uint32_t> in) {
  assert(out.size() == (in.size() * 4));
  size_t insert_idx = 0;
  for (size_t i = 0; i < in.size(); i++) {
    Util::put_le32(&out.data()[insert_idx], in[i]);
    insert_idx += 4;
  }
}

inline void le8_to_le16(std::span<uint16_t> out, std::span<uint8_t> in) {
  assert(out.size() == (in.size() / 2));
  size_t insert_idx = 0;
  for (size_t i = 0; i < in.size(); i += 2) {
    uint16_t val = Util::get_le16(&in.data()[i]);
    out[insert_idx] = val;
    insert_idx += 1;
  }
}

inline void le8_to_le32(std::span<uint32_t> out, std::span<uint8_t> in) {
  assert(out.size() == (in.size() / 4));
  size_t insert_idx = 0;
  for (size_t i = 0; i < in.size(); i += 4) {
    uint32_t val = Util::get_le32(&in.data()[i]);
    out[insert_idx] = val;
    insert_idx += 1;
  }
}
template <typename T>
inline void print_array(std::span<T> arr, size_t starting_index = 0) {
  printf("Printing array: [");
  for (size_t i = starting_index; i < arr.size(); i++) {
    printf("%d, ", arr[i]);
  }
  printf("]\n");
}

inline void print_schedule(const std::array<bool, 7> &schedule,
                           size_t starting_index = 0) {
  printf("Schedule: [");
  for (size_t i = starting_index; i < schedule.size(); ++i) {
    printf("%s", schedule[i] ? "true" : "false");
    if (i + 1 < schedule.size()) {
      printf(", ");
    }
  }
  printf("]\n");
}
} // namespace Util
