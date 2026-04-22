
#include "node_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include "portmacro.h"
#include "protocol.hpp"
#include <cstring>
#include <optional>

using CMD = Protocol::Command;

void NodeTask::run() {
  for (;;) {

    UartMessage frame{};
    if (xQueueReceive(this->incoming_queue, &frame, pdMS_TO_TICKS(100)) ==
        pdTRUE) {
      std::optional<UartMessage> response =
          this->self_node.handle_incoming_frame(frame);
      if (response) {
        xQueueSend(this->outgoing_queue, &(*response), portMAX_DELAY);
      }
    }
    if (this->self_node.get_status() == NodeStatus::WATERING) {
      this->self_node.process_watering_schedule();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
