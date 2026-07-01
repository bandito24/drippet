#include "head_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include "head.hpp"
#include "logger.hpp"
#include "portmacro.h"
#include "protocol.hpp"
#include "util.hpp"
#include <cstring>
#include <optional>

using CMD = Protocol::Command;
void HeadTask::run() {
  // Look for any nodes right away
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
      uint32_t tick_key = static_cast<uint32_t>(xTaskGetTickCount());
      std::optional<UartMessage> optMsg =
          this->headNode.process_pairing(response, tick_key);
      if (optMsg) {
        xQueueSend(this->outgoing_queue, &(*optMsg), portMAX_DELAY);
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
    uint8_t indicate_type = 0;
    if (before_status != after_status) {
      indicate_type |= static_cast<uint8_t>(EVENT_BITS::NODE_CHANGE);
    }
    this->headNode.process_external_requests();
    uint8_t queue_count = this->headNode.extRequestsManager.peek_event_count();
    if (queue_count) {
      indicate_type |= static_cast<uint8_t>(EVENT_BITS::EXT_RESPONSE);
    }
    if (indicate_type) {
      xTaskNotify(this->cccd_task_handle, indicate_type, eSetBits);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
