#include "uart_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "uart.hpp"

void UartTask::run() {

  for (;;) {
    // There's incoming uart data to read
    if (get_buffered_rx_length() != 0) {
      SizedReadBuffer incoming_buffer = this->uart.uart_read();
      if (incoming_buffer.length > 0) {
        size_t i = 0;
        bool reading = true;
        while (reading) {
          // Frame Buffer being extracted and queued
          std::optional<SizedFrameBuffer> frame_buffer =
              this->uart.parse_uart_read(incoming_buffer, i);
          if (!frame_buffer) {
            reading = false;
          } else {
            std::optional<UartMessage> uart_msg =
                this->uart.build_uart_message(frame_buffer->frame);
            if (!uart_msg) {
              reading = false;
            }
            i = frame_buffer->next_buffer_index;
            xQueueSend(this->to_head_queue, &uart_msg, pdMS_TO_TICKS(100));
          }
        }
      };
    }
    UartMessage msg{};
    if (xQueueReceive(this->from_head_queue, &msg, pdMS_TO_TICKS(5000))) {
      this->uart.write_bytes(msg);
    }
  }
}
