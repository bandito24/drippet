#include "constants.hpp"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "head_entrypoint.hpp"
#include "logger.hpp"
#include "queue.hpp"
#include "tasks/self_node_task.hpp"
#include "uart_task.hpp"
#include <memory>

extern "C" void app_main(void) {
  Logger::log_simple("STARTING OVER AGAIN!!");

  UartDriver driver{};
  ESP_ERROR_CHECK(driver.init());
  UartProtocol uart{driver};
  QueueManager q_manager{};
  q_manager.init();

  UartTask uart_task{uart, q_manager};
  uart_task.start();

  std::unique_ptr<SelfNodeTask> self_node_task = std::make_unique<SelfNodeTask>(
      q_manager.get_handle(QueueOrder::EXT_INCOMING),
      q_manager.get_handle(QueueOrder::EXT_OUTGOING), NodeMode::STANDARD);
  self_node_task->start();
  //  Duration we wait until switch to head mode
  vTaskDelay(pdMS_TO_TICKS(LISTEN_FOR_INIT_COMMS_DUR));
  std::unique_ptr<HeadEntrypointTask> head_entrypoint_task = nullptr;
  if (!self_node_task->heard_communication) {
    uart_task.init_dual_mode();
    head_entrypoint_task = std::make_unique<HeadEntrypointTask>(
        // NOTE: Self Node Task is destroyed and rebuilt in head_entrypoint
        std::move(self_node_task), q_manager);
    head_entrypoint_task->start();
  }
  for (;;) {

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
