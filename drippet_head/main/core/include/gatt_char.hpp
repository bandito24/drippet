#pragma once
#include <stdint.h>
class GattChar {

public:
  uint16_t chr_handle;
};

class NodeConfigChar final : public GattChar {
public:
  using GattChar::GattChar;
};

class NodeDescChar final : public GattChar {
public:
  using GattChar::GattChar;
};

class ExtReqChar final : public GattChar {
public:
  using GattChar::GattChar;
};

class SysConfigChar final : public GattChar {
public:
  using GattChar::GattChar;
};
