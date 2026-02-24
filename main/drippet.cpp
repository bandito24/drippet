#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "node.hpp"
#include "queues/queue.hpp"
#include "tasks/uart_task.hpp"

#include "tasks/head_task.hpp"
#include "time.hpp"
#include "uart.hpp"
#include <memory>
extern "C" void app_main(void) {
  EspUart espUart{};
  ESP_ERROR_CHECK(espUart.init());

  // For Queueing Incoming Messages From Peripherals and Sending Head Queued
  // Messages
  Queue incoming_queue(10, sizeof(UartMessage));
  incoming_queue.init();
  Queue outgoing_queue(10, sizeof(UartMessage));
  outgoing_queue.init();

  UartTask uart_task{espUart, incoming_queue.get_handle(),
                     outgoing_queue.get_handle()};
  uart_task.start_and_notify();

  // For Queueing Outgoing Messages and Reading Uart Queued Messages
  MainValve main_valve{};
  std::unique_ptr<iClock> clock = initialize_clock();
  HeadTask head_task{main_valve, *clock, incoming_queue.get_handle(),
                     outgoing_queue.get_handle()};

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
