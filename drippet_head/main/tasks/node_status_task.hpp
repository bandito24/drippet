#pragma once
#include "constants.hpp"
#include "esp_switch.hpp"
#include "soc/gpio_num.h"
#include "task.hpp"

class NodeStatusTask : public Task {

private:
  const NodeStatus &self_node_status;

public:
  NodeStatusTask(const NodeStatus &_self_node_status,
                 gpio_num_t _indication_gpio)
      : Task("SELF_NODE_STATUS", 4096, 1), self_node_status{_self_node_status},
        led_indication(_indication_gpio, GpioActiveLevel::ACTIVE_HIGH){};

  LedIndication led_indication;
  // Doing this when led indication transfers to head in case of head node
  bool disable_indication = false;
  void disable_led_indication() {
    led_indication.disable();
    this->disable_indication = true;
  }

protected:
  void run() override;
};
