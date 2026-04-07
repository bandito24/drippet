#include "head_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include "head.hpp"
#include "portmacro.h"
#include "protocol.hpp"
#include <cstring>

using CMD = Protocol::Command;
void HeadTask::run() {
  for (;;) {
    all_node_status_t before_status = this->headNode.get_node_statuses();

    UartMessage frame{};
    if (xQueueReceive(this->incoming_queue, &frame, pdMS_TO_TICKS(100)) ==
        pdTRUE) {
      std::optional<UartMessage> response =
          this->headNode.handle_incoming_frame(frame);
      if (response) {
        xQueueSend(this->outgoing_queue, &(*response), portMAX_DELAY);
      }
    }

    headNode.process_watering_schedule();

    if (headNode.get_head_status() == HeadStatus::WATERING_CMDS) {
      std::optional<UartMessage> msg = headNode.next_watering_frame();
      // If nullptr headNode updates its status away from watering_cmds
      if (msg) {
        UartMessage m = *msg;
        xQueueSend(this->outgoing_queue, &m, portMAX_DELAY);
      }
    }

    all_node_status_t after_status = this->headNode.get_node_statuses();
    if (before_status != after_status) {
      Logger::log_simple("Should be sending an indication");
      xTaskNotifyGive(this->cccd_task_handle);
    }
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
}
