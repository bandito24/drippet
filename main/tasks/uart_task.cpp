#include "uart_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "uart.hpp"

void UartTask::run() {

  Protocol::Frame buffer{};
  for (;;) {
    if (xQueueReceive(this->from_node_queue, buffer, pdMS_TO_TICKS(50)) ==
        pdPASS) {

      if (UartFunctions::validate_frame(buffer) != ParseResult::Ok) {
        return;
      }
      UartMessage frame = UartFunctions::reconstruct_uart_message(buffer);
      xQueueSend(this->to_head_queue, &frame, portMAX_DELAY);
    }
  }
}
