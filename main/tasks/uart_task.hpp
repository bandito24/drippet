#pragma once
#include "freertos/idf_additions.h"
#include "task.hpp"
#include "uart.hpp"

class UartTask : public Task {
public:
  UartTask(Uart &uart_class, QueueHandle_t fromNodeHandle,
           QueueHandle_t toHeadHandle)
      : Task("UART", 4096, 2), uart{uart_class},
        from_node_queue(fromNodeHandle), to_head_queue(toHeadHandle){};
  Uart &uart;

protected:
  void run() override;

private:
  QueueHandle_t from_node_queue;
  QueueHandle_t to_head_queue;
};
