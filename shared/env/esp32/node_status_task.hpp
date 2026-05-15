#pragma once
#include "constants.hpp"
#include "esp_switch.hpp"
#include "soc/gpio_num.h"
#include "switch.hpp"
#include "task.hpp"

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

class NodeStatusTask : public Task {

public:
  NodeStatusTask(const NodeStatus &_self_node_status,
                 gpio_num_t _indication_gpio)
      : Task("SELF_NODE_STATUS", 1024, 1), self_node_status{_self_node_status},
        led_indication(_indication_gpio, GpioActiveLevel::ACTIVE_HIGH) {};

protected:
  void run() override;

private:
  const NodeStatus &self_node_status;
  LedIndication led_indication;
};
