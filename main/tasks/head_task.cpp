#include "head_task.hpp"
#include "config.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "head.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "portmacro.h"
#include "protocol.hpp"
#include <cstdint>
#include <cstring>
#include <memory>

using CMD = Protocol::Command;
void HeadTask::run() {
  UartMessage frame{};
  for (;;) {
    if (xQueueReceive(this->incoming_queue, &frame, pdMS_TO_TICKS(100)) ==
        pdTRUE) {
      if (frame.command == CMD::DISCOVERY) {
        // Newly connected Node broadcasts DISCOVERY with key. Head Node
        // responds with ADDRESSING with address and received key (for
        // identification). Node responds with ADDRESSING and key for final
        // acknoledgement
        NodeKey_t key = frame.data[0];
        UartMessage msg{};
        std::optional<config::Address> address =
            this->headNode.create_node_pending(key);
        if (address) {
          // No Address means key is recognized and it's a duplicate request
          if (address ==
              config::max_nodes) { // Means system cannot index another
                                   // spot for it. Maybe
                                   // make it sleep for a while
            msg = this->headNode.terminate_endpoint(key);
          } else {
            msg = this->headNode.create_addressing_frame(key, *address);
          }
          xQueueSend(this->outgoing_queue, &msg, portMAX_DELAY);
        } else {
          Logger::log_simple("address is nullopt");
        }
      } else if (frame.command ==
                 CMD::ADDRESSING) { // Means the node is attempting to reple
                                    // with the same address and key for final
                                    // confirmation

        NodeKey_t key = frame.data[0];
        NodeLinkStatus status =
            this->headNode.confirm_node_pending(key, frame.address);
        if (status == NodeLinkStatus::LINK_OK) {
          Logger::log_simple("Node Successfully connected");
        } else {
          Logger::log_error("Node connect unsuccessful");
        }
        // If unsuccessful do nothing and node will reattempt the broadcast
        // after short period
      } else {
        printf("unknown command of %d", static_cast<int>(frame.command));
      }
      frame = {};
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
