#pragma once
#include "FreeRTOSConfig.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
class Task {
public:
  Task(const char *const process_name, configSTACK_DEPTH_TYPE stack_depth,
       UBaseType_t priority)
      : pcName(process_name), uxStackDepth(stack_depth), uxPriority(priority) {
    task_handle = nullptr;
  }

  Task(const char *const process_name, configSTACK_DEPTH_TYPE stack_depth,
       UBaseType_t priority, TaskHandle_t handle)

      : pcName(process_name), uxStackDepth(stack_depth), uxPriority(priority),
        task_handle{handle} {};

  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;
  virtual ~Task() = default;
  BaseType_t start();
  void notify();
  BaseType_t start_and_notify();
  TaskHandle_t get_handle() const { return this->task_handle; }

protected:
  virtual void run() { printf("RUNNING BASE TASK NONTASK"); };
  TaskHandle_t getHandle() const { return task_handle; }

private:
  const char *const pcName;
  configSTACK_DEPTH_TYPE uxStackDepth;
  UBaseType_t uxPriority;
  TaskHandle_t task_handle;
  static void run_task(void *pvParameters);
};
