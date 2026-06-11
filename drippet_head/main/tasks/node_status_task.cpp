
#include "node_status_task.hpp"
#include "constants.hpp"
#include "protocol_types.hpp"

void NodeStatusTask::run() {
  this->led_indication.init();
  for (;;) {
    if (!this->disable_indication) {
      switch (this->self_node_status) {
      case NodeStatus::INITIALIZING:
        this->led_indication.toggle();
        break;
      case NodeStatus::ERR:
        this->led_indication.disable();
        break;
      default:
        this->led_indication.enable();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
