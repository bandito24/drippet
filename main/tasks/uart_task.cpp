#include "uart_task.hpp"
#include "driver.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "protocol.hpp"
#include <optional>

void UartTask::run() {

  for (;;) {
    // There's incoming uart data to read
    if (get_buffered_rx_length() != 0) {

      SizedReadBuffer incoming_buffer = this->uart.receive_bytes();
      if (incoming_buffer.length > 0) {
        LocalReadBuffer &bytes = incoming_buffer.content;
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
          xQueueSend(this->to_head_queue, &uart_msg, pdMS_TO_TICKS(100));
        }
      }
    };

    UartMessage msg{};
    if (xQueueReceive(this->from_head_queue, &msg, pdMS_TO_TICKS(5000))) {
      SizedFrameBuffer buffer = this->uart.prepare_bytes(msg);
      this->uart.write_bytes(buffer);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
