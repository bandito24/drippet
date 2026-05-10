#pragma once
#include "constants.hpp"
#include "driver/gpio.h"
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "switch.hpp"
class EspSolenoid final : public SolenoidValve {

public:
  EspSolenoid(gpio_num_t gpio_output) : gpio{gpio_output} {};
  Esp_Err_t init() override {
    esp_err_t rc = gpio_set_direction(this->gpio, GPIO_MODE_OUTPUT);
    rc |= gpio_set_level(this->gpio, 0);
    return rc;
  };
  Esp_Err_t enable() override { return gpio_set_level(this->gpio, 1); };
  Esp_Err_t disable() override { return gpio_set_level(this->gpio, 0); };
  bool is_enabled() const override { return gpio_get_level(this->gpio) == 1; };

private:
  gpio_num_t gpio;
};
