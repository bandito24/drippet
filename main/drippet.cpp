#include "clock.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "node.hpp"
#include "queues/queue.hpp"
#include "tasks/head_task.hpp"
#include "tasks/uart_task.hpp"
#include <iostream>
#include <memory>
extern "C" void app_main(void) {
  setenv("TZ", "UTC", 1);
  tzset();

  UartDriver driver{};
  ESP_ERROR_CHECK(driver.init());
  UartProtocol uart{driver};
  SystemClock sys_clock{};
  Esp32Clock clock{sys_clock};
  clock.set_time(2026, 1, 1, 0, 0);

  //// For Queueing Incoming Messages From Peripherals and Sending Head Queued
  //// Messages
  Queue incoming_queue(10, sizeof(UartMessage));
  incoming_queue.init();
  Queue outgoing_queue(10, sizeof(UartMessage));
  outgoing_queue.init();

  UartTask uart_task{uart, outgoing_queue.get_handle(),
                     incoming_queue.get_handle()};
  uart_task.start();

  MainValve main_valve{};
  HeadTask head_task{main_valve, clock, incoming_queue.get_handle(),
                     outgoing_queue.get_handle()};
  head_task.start();

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
