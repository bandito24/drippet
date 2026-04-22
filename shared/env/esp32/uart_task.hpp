#include "protocol.hpp"
#pragma once
#include "freertos/idf_additions.h"
#include "task.hpp"

class UartTask : public Task {
public:
  UartTask(UartProtocol &uart_protocal, QueueHandle_t outgoingQueue,
           QueueHandle_t incomingQueue)
      : Task("UART", 4096, 2), uart{uart_protocal},
        outgoing_queue(outgoingQueue), incoming_queue(incomingQueue){};
  UartProtocol &uart;

protected:
  void run() override;

private:
  QueueHandle_t outgoing_queue;
  QueueHandle_t incoming_queue;
};
