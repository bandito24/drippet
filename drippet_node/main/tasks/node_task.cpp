
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
      std::optional<UartMessage> response = std::nullopt;
      // TODO: implement the logic of responding to an incoming message
      if (response) {
        xQueueSend(this->outgoing_queue, &(*response), portMAX_DELAY);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
