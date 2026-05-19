#include "head_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include "head.hpp"
#include "portmacro.h"
#include "protocol.hpp"
#include "util.hpp"
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
      }
    }

    if (headNode.get_head_status() == HeadStatus::PAIRING) {

      Logger::log_simple("no reponse count = %d", this->no_response_count);
      // Stop pairing if there has been no response
      if (this->no_response_count >= STOP_PAIRING_COUNT) {
        headNode.end_pairing_mode();
      } else {
        if (!response) {
          this->no_response_count++;
          auto key =
              Util::serialize_key(static_cast<uint32_t>(xTaskGetTickCount()));
          response = UartMessage{.address = ADDR_UNSET,
                                 .command = CMD::DISCOVERY,
                                 .data = {key[0], key[1]}};
          xQueueSend(this->outgoing_queue, &response, portMAX_DELAY);
        } else {
          this->no_response_count = 0;
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
    }

    all_node_status_t after_status = this->headNode.get_node_statuses();
    if (before_status != after_status) {
      xTaskNotifyGive(this->cccd_task_handle);
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
