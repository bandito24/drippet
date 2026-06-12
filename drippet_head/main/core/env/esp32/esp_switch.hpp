
#pragma once
#include "constants.hpp"
#include "driver/gpio.h"
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "logger.hpp"
#include "switch.hpp"

enum class GpioActiveLevel { ACTIVE_LOW, ACTIVE_HIGH };

class EspSwitch : public SolenoidValve {

public:
  EspSwitch(gpio_num_t gpio_output, GpioActiveLevel _active_level)
      : gpio{gpio_output},
        active_level(_active_level == GpioActiveLevel::ACTIVE_HIGH ? 1 : 0){};
  ~EspSwitch() { this->disable(); }
  Esp_Err_t init() override {

    gpio_reset_pin(this->gpio);
    esp_err_t rc = gpio_set_direction(this->gpio, GPIO_MODE_OUTPUT);
    rc |= gpio_set_level(this->gpio, !this->active_level);
    this->initialized = true;
    return rc;
  };
  Esp_Err_t enable() override {
    assert(this->initialized);
    Esp_Err_t rc = gpio_set_level(this->gpio, this->active_level);
    if (rc == ESP_OK) {
      this->enabled = true;
    }
    return rc;
  };
  Esp_Err_t disable() override {
    Esp_Err_t rc = gpio_set_level(this->gpio, !this->active_level);
    if (rc == ESP_OK) {
      this->enabled = false;
    }
    return rc;
  };
  bool is_enabled() const override { return this->enabled; };

protected:
  gpio_num_t gpio;
  int active_level;
  bool enabled = false;
  bool initialized = false;
};

class LedIndication : public EspSwitch {
public:
  using EspSwitch::EspSwitch;
  Esp_Err_t toggle() {
    if (this->is_enabled()) {
      return this->disable();
    } else {
      return this->enable();
    }
  }
};
