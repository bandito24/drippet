#include "driver/gpio.h"
#include "esp_err.h"
#include "hal/gpio_types.h"

#include "config.hpp"
#include "constants.hpp"
#include "esp32config.hpp"
#include "esp_switch.hpp"

#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "self_node_task.hpp"

#include "logger.hpp"
#include "portmacro.h"
#include "protocol.hpp"
#include "self_node.hpp"
#include "soc/gpio_num.h"
#include "switch.hpp"
#include <cstring>
#include <memory>
#include <optional>

using CMD = Protocol::Command;

void SelfNodeTask::run() {

  SolenoidPtr solenoid =
      std::make_unique<EspSwitch>(SOLENOID_PIN, GpioActiveLevel::ACTIVE_HIGH);
  ESP_ERROR_CHECK(solenoid->init());
  SolenoidPtr downstreamPower = std::make_unique<EspSwitch>(
      DOWNSTREAM_PWR_SWITCH, GpioActiveLevel::ACTIVE_HIGH);
  ESP_ERROR_CHECK(downstreamPower->init());
  this->self_node = std::make_unique<SelfNode>(
      this->steady_clock, std::move(solenoid), std::move(downstreamPower));

  // this->self_node = std::make_unique<SelfNode>(this->steady_clock);

  if (this->mode == NodeMode::STANDARD) {
    // NOTE: we use the same gpio pin when its dual mode so don't need this
    // task
    this->led_status_indication = std::make_unique<NodeStatusTask>(
        this->self_node->get_status(), STATUS_LED);
    this->led_status_indication->start();
  }

  Logger::log_simple("Restarting the task");
  // Loop start
  for (;;) {
    if (delete_task_requested) {
      break;
    }

    UartMessage frame{};
    if (xQueueReceive(this->incoming_queue, &frame, 0)) {
      std::optional<UartMessage> response =
          this->self_node->handle_incoming_frame(frame);
      if (response) {
        xQueueSend(this->outgoing_queue, &(*response), portMAX_DELAY);
      }
      this->heard_communication = true;
    }
    if (this->self_node->get_status() == NodeStatus::WATERING) {
      Logger::log_simple("node is currently watering");
      this->self_node->process_watering_schedule();
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
  this->task_stopped = true;
}
