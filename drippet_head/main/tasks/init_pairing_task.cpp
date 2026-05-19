#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "init_pariring_task.hpp"
#include "logger.hpp"
#include "portmacro.h"
void InitParingTask::run() {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    Logger::log_simple("it has been done");
    this->head_status_cb();
  }
}
