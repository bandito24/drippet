
#pragma once
#include "esp_switch.hpp"
#include "head.hpp"
#include "soc/gpio_num.h"
#include "task.hpp"

class HeadStatusTask : public Task {

public:
  HeadStatusTask(const HeadStatus &_head_status, gpio_num_t _indication_gpio)
      : Task("HEAD_STATUS", 1024, 1), head_status{_head_status},
        led_indication(_indication_gpio, GpioActiveLevel::ACTIVE_HIGH){};

protected:
  void run() override;

private:
  const HeadStatus &head_status;
  LedIndication led_indication;
};
