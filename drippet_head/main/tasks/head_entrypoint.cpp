#include "head_entrypoint.hpp"
#include "esp_err.h"
void HeadEntrypointTask::run() {

  setenv("TZ", "UTC", 1);
  tzset();
  this->clock.set_time(2026, 1, 1, 2, 0);

  // NOTE: for debugging, delete
  vTaskDelay(pdMS_TO_TICKS(5000));
  auto rc = head_node.set_node_duration(0, 5);

  clock.set_next_phase_start_time(1, 0);
  if (rc != 0) {
    Logger::log_error("RC is not 0 for set node duration");
  }

  for (;;) {

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
