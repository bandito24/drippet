#include "uart_task.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "protocol.hpp"
#include <optional>

void UartTask::run() {

  for (;;) {
    SizedReadBuffer incoming_buffer = this->uart.receive_bytes();
    if (incoming_buffer.length > 0) {

      size_t start_index = 0;
      while (true) {
        std::optional<IndexedFrame> indexedFrame =
            this->uart.parse_uart_read(incoming_buffer, start_index);
        if (!indexedFrame) {
          break;
        }
        start_index += indexedFrame->i.length;
        std::optional<UartMessage> uart_msg =
            this->uart.build_uart_message(indexedFrame->frame);
        if (!uart_msg) {
          continue;
        }
        xQueueSend(this->incoming_queue, &uart_msg, 0);
      }
    }

    UartMessage msg{};
    if (xQueueReceive(this->outgoing_queue, &msg, 0)) {
      if (this->dual_mode_active) {
        xQueueSend(this->self_node_incoming_queue, &msg, 0);
      }
      SizedFrameBuffer buffer = this->uart.prepare_bytes(msg);
      this->uart.write_bytes(buffer);
    }

    UartMessage msg_local{};
    if (xQueueReceive(this->self_node_outgoing_queue, &msg_local, 0)) {
      xQueueSend(this->incoming_queue, &msg_local, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
