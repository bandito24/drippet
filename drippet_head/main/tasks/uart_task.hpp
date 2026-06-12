#pragma once

#include "freertos/idf_additions.h"
#include "protocol.hpp"
#include "queue.hpp"
#include "task.hpp"

using Qt = QueueHandle_t;
class UartTask : public Task {
public:
  UartTask(UartProtocol &uart_protocal, QueueManager &qManager)
      : Task("UART", 4096, 2), uart{uart_protocal},
        outgoing_queue(qManager.get_handle(QueueOrder::EXT_OUTGOING)),
        incoming_queue(qManager.get_handle(QueueOrder::EXT_INCOMING)),
        self_node_outgoing_queue(
            qManager.get_handle(QueueOrder::INTERNAL_OUTGOING)),
        self_node_incoming_queue(
            qManager.get_handle(QueueOrder::INTERNAL_INCOMING)){};
  UartProtocol &uart;
  void init_dual_mode() { this->dual_mode_active = true; }

protected:
  void run() override;

private:
  Qt outgoing_queue;
  Qt incoming_queue;
  Qt self_node_outgoing_queue;
  Qt self_node_incoming_queue;
  // This indicates whether the uart task needs to send messages internally
  // instead of over uart. Used when the device is the head and first node
  bool dual_mode_active = false;
};
