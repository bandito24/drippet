#include "driver/gpio.h"
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "logger.hpp"

#include "config.hpp"
#include "constants.hpp"
#include "esp32config.hpp"
#include "esp_solenoid.hpp"

#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "self_node_task.hpp"

#include "portmacro.h"
#include "protocol.hpp"
#include "soc/gpio_num.h"
#include "switch.hpp"
#include <cstring>
#include <memory>
#include <optional>

using CMD = Protocol::Command;

void SelfNodeTask::run() {

  SolenoidGrouping solenoids = {};
  for (size_t i = 0; i < config::node_hose_count; i++) {
    auto ptr = std::make_unique<EspSolenoid>(EspConfig::node_gpio_valves.at(i));
    solenoids.at(i) = std::move(ptr);
  }
  this->solenoid_manager =
      std::make_unique<SolenoidManager>(std::move(solenoids));
  ESP_ERROR_CHECK(this->solenoid_manager->initialize_solenoids());
  this->self_node =
      std::make_unique<SelfNode>(this->steady_clock, *this->solenoid_manager);

  uint32_t i = 0;
  for (;;) {

    UartMessage frame{};
    if (xQueueReceive(this->incoming_queue, &frame, 0)) {
      std::optional<UartMessage> response =
          this->self_node->handle_incoming_frame(frame);
      if (response) {
        xQueueSend(this->outgoing_queue, &(*response), portMAX_DELAY);
      }
    }
    if (this->self_node->get_status() == NodeStatus::WATERING) {
      this->self_node->process_watering_schedule();
    }
    // NOTE: for test, remove
    for (int j = 0; j < config::node_hose_count; j++) {
      Logger::log_simple("hose %d", j);
      solenoid_manager->activate_solenoid(j);
      vTaskDelay(pdMS_TO_TICKS(700));
    }
    // NOTE: delete this
    UartMessage msg{.address = ADDR_UNSET, .command = CMD::ACK};
    xQueueSend(this->outgoing_queue, &msg, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
