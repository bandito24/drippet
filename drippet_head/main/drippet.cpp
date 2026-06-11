#include "clock.hpp"
#include "constants.hpp"
#include "esp32config.hpp"
#include "esp_err.h"
#include "esp_switch.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "head_entrypoint.hpp"
#include "head_status_task.hpp"
#include "logger.hpp"
#include "nvs_storage.hpp"
#include "queue.hpp"
#include "tasks/bluetooth_task.hpp"
#include "tasks/head_task.hpp"
#include "tasks/self_node_task.hpp"
#include "uart_task.hpp"
#include <memory>

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

  // NOTE: this is only used when the node switches to central mode
  Queue local_incoming_queue(10, sizeof(UartMessage));
  Queue local_outgoing_queue(10, sizeof(UartMessage));
  local_incoming_queue.init();
  local_outgoing_queue.init();

  UartTask uart_task{
      uart, outgoing_queue.get_handle(), incoming_queue.get_handle(),
      local_outgoing_queue.get_handle(), local_incoming_queue.get_handle()};
  uart_task.start();

  Logger::log_simple("GOING IN FOR NODE");
  std::unique_ptr<SelfNodeTask> self_node_task = std::make_unique<SelfNodeTask>(
      incoming_queue.get_handle(), outgoing_queue.get_handle());
  self_node_task->start();
  // Duration we wait until switch to head mode
  vTaskDelay(pdMS_TO_TICKS(1000));
  std::unique_ptr<HeadEntrypointTask> head_entrypoint_task = nullptr;
  if (!self_node_task->heard_communication) {

    Logger::log_simple("GOING TO HEAD MODE");
    self_node_task->disable_led();
    vTaskSuspend(self_node_task->get_handle());
    self_node_task->change_queues_for_local_node(
        local_incoming_queue.get_handle(), local_outgoing_queue.get_handle());
    vTaskResume(self_node_task->get_handle());
    uart_task.init_dual_mode();
    head_entrypoint_task = std::make_unique<HeadEntrypointTask>(
        incoming_queue.get_handle(), outgoing_queue.get_handle());
    head_entrypoint_task->start();
  }
  for (;;) {

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
