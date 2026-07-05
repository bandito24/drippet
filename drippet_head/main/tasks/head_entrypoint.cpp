#include "head_entrypoint.hpp"
#include "esp_err.h"
void HeadEntrypointTask::run() {

  setenv("TZ", "UTC", 1);
  tzset();
  this->clock.set_time(2026, 1, 1, 2, 0);
  clock.set_next_phase_start_time(1, 0);

  this->init();
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  }
}
void HeadEntrypointTask::reset_node() {

  this->node_task->request_delete_task();
  int timeout = 1000; // 10 seconds
  while (this->node_task->is_task_running() && timeout-- > 0) {
    vTaskDelay(pdMS_TO_TICKS(10)); // !0 Seconds
  }
  if (timeout <= 0) {
    Logger::log_error("Self Node Task was unable to safely stop");
  }
  Logger::log_simple("About to recreate the selfNodeTask");
  this->node_task = std::make_unique<SelfNodeTask>(
      this->q_manager.get_handle(QueueOrder::INTERNAL_INCOMING),
      this->q_manager.get_handle(QueueOrder::INTERNAL_OUTGOING),
      NodeMode::DUAL);

  Logger::log_simple("Was able to recreate the selfNodeTask");
  this->node_task->start();

  Logger::log_simple("Started the node task function");
}
