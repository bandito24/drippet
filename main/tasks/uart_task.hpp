#include "protocol.hpp"
#pragma once
#include "freertos/idf_additions.h"
#include "task.hpp"

class UartTask : public Task {
public:
  UartTask(UartProtocol &uart_protocal, QueueHandle_t fromHeadHandle,
           QueueHandle_t toHeadHandle)
      : Task("UART", 4096, 2), uart{uart_protocal},
        from_head_queue(fromHeadHandle), to_head_queue(toHeadHandle){};
  UartProtocol &uart;

protected:
  void run() override;

private:
  QueueHandle_t from_head_queue;
  QueueHandle_t to_head_queue;
};
