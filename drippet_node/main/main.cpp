#include "driver.hpp"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "logger.hpp"
#include "queue.hpp"
#include "self_node_task.hpp"
#include "uart_task.hpp"

extern "C" void app_main(void) {

  UartDriver driver{};
  ESP_ERROR_CHECK(driver.init());

  UartProtocol uart{driver};

  //// For Queueing Incoming Messages From Peripherals and Sending Head Queued
  //// Messages
  Queue incoming_queue(10, sizeof(UartMessage));
  incoming_queue.init();
  Queue outgoing_queue(10, sizeof(UartMessage));
  outgoing_queue.init();

  UartTask uart_task{uart, outgoing_queue.get_handle(),
                     incoming_queue.get_handle()};
  uart_task.start();

  SelfNodeTask self_node_task{incoming_queue.get_handle(),
                              outgoing_queue.get_handle()};
  self_node_task.start();
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
