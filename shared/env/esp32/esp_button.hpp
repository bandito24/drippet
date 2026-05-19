
#pragma once
#include "constants.hpp"
#include "driver/gpio.h"
#include "esp32config.hpp"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "init_pariring_task.hpp"
#include "logger.hpp"
#include "soc/gpio_num.h"
#include "switch.hpp"

#include <functional>

class EspButton : public Button {
public:
  EspButton(gpio_num_t _gpio, std::function<void()> _intr_cb)
      : gpio{_gpio}, pairing_task(_intr_cb) {
    this->pairing_task.start();
    this->pairing_handle = this->pairing_task.get_handle();
  };
  Esp_Err_t init();
  static void IRAM_ATTR gpio_isr_handler(void *arg);

private:
  gpio_num_t gpio;
  TaskHandle_t pairing_handle{};
  InitParingTask pairing_task;
};
