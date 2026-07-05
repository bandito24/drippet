#pragma once
#include <cstddef>
#include <stdint.h>

template <typename T, typename Tag> struct NamedType {
  explicit NamedType(T value) : _value{value} {};
  const T value() { return this->_value; }

private:
  T _value;
};

struct Hour : public NamedType<uint8_t, struct HourTag> {
  explicit Hour(uint8_t value) : NamedType(value) {}
};

struct Minute : public NamedType<uint8_t, struct MinuteTag> {
  explicit Minute(uint8_t value) : NamedType(value) {}
};

struct NodeIndex : public NamedType<uint8_t, struct NodeIndexTag> {
  explicit NodeIndex(uint8_t value) : NamedType(value) {}
};
