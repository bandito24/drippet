#include "head_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "head.hpp"
#include "portmacro.h"
#include "protocol.hpp"
#include <cstring>

using CMD = Protocol::Command;
void HeadTask::run() {
  UartMessage frame{};
  for (;;) {
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
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
