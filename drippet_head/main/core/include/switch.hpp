#pragma once
#include "config.hpp"
#include "constants.hpp"
#include <cassert>
#include <memory>
struct Switch {
  virtual Esp_Err_t enable() = 0;
  virtual Esp_Err_t init() = 0;
  virtual Esp_Err_t disable() = 0;
  virtual bool is_enabled() const = 0;
  virtual ~Switch() = default;
};
class SolenoidValve : public Switch {};

struct Button {};
