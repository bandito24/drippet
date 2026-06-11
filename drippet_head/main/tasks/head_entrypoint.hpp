
#pragma once
#include "bluetooth_task.hpp"
#include "clock.hpp"
#include "constants.hpp"
#include "freertos/idf_additions.h"
#include "head.hpp"
#include "head_status_task.hpp"
#include "head_task.hpp"
#include "nvs_storage.hpp"
#include "task.hpp"
#include <memory>
#include <optional>

class HeadEntrypointTask : public Task {

public:
  HeadEntrypointTask(QueueHandle_t incomingQueueHandle,
                     QueueHandle_t outgoingQueueHandle)
      : Task("HEAD_ENTRYPOINT", 4096, 2) {

    this->storage.init();
    ESP_ERROR_CHECK(this->ble_task.init_stack());
    this->ble_task.start();
    this->status_task.start();
    head_task = std::make_unique<HeadTask>(head_node, incomingQueueHandle,
                                           outgoingQueueHandle,
                                           ble_task.get_cccd_subtask_handle());
    head_task->start();
  }

protected:
  void run() override;

private:
  NvsStorage storage{};
  SystemClock sys_clock{};
  Esp32Clock clock{DEV_PHASE_LEN, sys_clock};
  Head head_node{clock, storage};
  BluetoothTask ble_task{head_node};
  std::unique_ptr<HeadTask> head_task = nullptr;
  HeadStatusTask status_task{head_node.get_head_status(), STATUS_LED};
};
