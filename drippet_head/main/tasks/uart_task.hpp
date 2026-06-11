#include "protocol.hpp"
#pragma once
#include "freertos/idf_additions.h"
#include "task.hpp"

using Qt = QueueHandle_t;
class UartTask : public Task {
public:
  UartTask(UartProtocol &uart_protocal, Qt outgoingQueue, Qt incomingQueue,
           Qt selfNodeOutgoingQueue, Qt selfNodeIncomingQueue)
      : Task("UART", 4096, 2), uart{uart_protocal},
        outgoing_queue(outgoingQueue), incoming_queue(incomingQueue),
        self_node_outgoing_queue(selfNodeOutgoingQueue),
        self_node_incoming_queue(selfNodeIncomingQueue){};
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
