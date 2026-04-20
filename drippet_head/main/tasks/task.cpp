#include "task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"

BaseType_t Task::start() {
  return xTaskCreate(run_task, pcName, uxStackDepth, this, uxPriority,
                     &task_handle);
}
void Task::notify() { xTaskNotifyGive(this->task_handle); }
BaseType_t Task::start_and_notify() {
  BaseType_t base = this->start();
  this->notify();
  return base;
}

void Task::run_task(void *pvParameters) {
  Task *task = static_cast<Task *>(pvParameters);
  task->run();
  vTaskDelete(NULL);
}
