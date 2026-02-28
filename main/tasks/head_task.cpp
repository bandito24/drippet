#include "head_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "logger.hpp"
#include "node.hpp"
#include "portmacro.h"
#include "uart.hpp"
#include <cstdint>
#include <cstring>
#include <memory>

using CMD = Protocol::Command;
void HeadTask::run() {
  UartMessage frame{};
  for (;;) {
    if (xQueueReceive(this->incoming_queue, &frame, pdMS_TO_TICKS(100)) ==
        pdTRUE) {
      Logger::log_simple("SOMETHING IS COMING IN");

      if (frame.command == CMD::DISCOVERY) {
        // Newly connected Node broadcasts DISCOVERY with key. Head Node
        // responds with ADDRESSING with address and received key (for
        // identification). Node responds with ADDRESSING and key for final
        // acknoledgement
        uint16_t key = frame.data[0];
        printf("key received is %d", key);
        UartMessage msg = headNode.create_addressing_frame(key);
        headNode.pending_node = {.address = msg.address, .key = key};
        xQueueSend(this->outgoing_queue, &msg, portMAX_DELAY);
      } else if (frame.command == CMD::ADDRESSING) {
        uint16_t key = frame.data[0];
        if (frame.address != headNode.pending_node.address ||
            key != headNode.pending_node.key) {
          headNode.reset_pending_node();
        } else {
          NodeTypes::Node_Link new_node =
              std::make_unique<Node>(frame.address, key);
          headNode.add_node(std::move(new_node));
        }
      }
      frame = {};
    }

    // Logger::log_simple("TRYING TO SEND");
    // uint16_t test_key = 24;
    // UartMessage msg = headNode.create_addressing_frame(test_key);
    // headNode.pending_node = {.address = msg.address, .key = test_key};
    // xQueueSend(this->outgoing_queue, &msg, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
