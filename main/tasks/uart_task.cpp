#include "uart_task.hpp"
#include "driver.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "logger.hpp"
#include "protocol.hpp"
#include <optional>

void UartTask::run() {

  Logger::log_simple("STARTING UP THE TASK");
  for (;;) {
    SizedReadBuffer incoming_buffer = this->uart.receive_bytes();
    if (incoming_buffer.length > 0) {
      printf("size of %d\n", incoming_buffer.length);

      // for (size_t i = 0; i < 13; i++) {
      //   printf("%d\n", incoming_buffer.content[i]);
      // }
      Logger::log_simple("something is comming in");
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
        Logger::log_simple("queued it up");
        xQueueSend(this->incoming_queue, &uart_msg, pdMS_TO_TICKS(100));
      }
    }

    UartMessage msg{};
    if (xQueueReceive(this->outgoing_queue, &msg, pdMS_TO_TICKS(5000))) {
      SizedFrameBuffer buffer = this->uart.prepare_bytes(msg);
      this->uart.write_bytes(buffer);
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
