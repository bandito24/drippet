#include "head_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include "head.hpp"
#include "portmacro.h"
#include "protocol.hpp"
#include <cstring>
#include <optional>

using CMD = Protocol::Command;
void HeadTask::run() {
  for (;;) {
    std::optional<UartMessage> response = std::nullopt;
    all_node_status_t before_status = this->headNode.get_node_statuses();

    UartMessage frame{};
    if (xQueueReceive(this->incoming_queue, &frame, pdMS_TO_TICKS(100)) ==
        pdTRUE) {
      response = this->headNode.handle_incoming_frame(frame);
      if (response) {
        xQueueSend(this->outgoing_queue, &(*response), portMAX_DELAY);
        this->no_response_count = 0;
      } else {
        this->no_response_count += 1;
      }
    }

    if (headNode.get_head_status() == HeadStatus::PAIRING) {
      // Stop pairing if there has been no response
      if (this->no_response_count == STOP_PAIRING_COUNT) {
        headNode.end_pairing_mode();
      } else {
        if (!response) {
          response =
              UartMessage{.address = ADDR_UNSET, .command = CMD::DISCOVERY};
          xQueueSend(this->outgoing_queue, &response, portMAX_DELAY);
        }
      }
    } else {
      headNode.process_watering_schedule();
      if (headNode.get_head_status() == HeadStatus::WATERING_CMDS) {
        std::optional<UartMessage> msg = headNode.next_watering_frame();
        // If nullptr headNode updates its status away from watering_cmds
        if (msg) {
          UartMessage m = *msg;
          xQueueSend(this->outgoing_queue, &m, portMAX_DELAY);
        }
      }
      headNode.print_node_durations();
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    all_node_status_t after_status = this->headNode.get_node_statuses();
    if (before_status != after_status) {
      xTaskNotifyGive(this->cccd_task_handle);
    }
  }
}
