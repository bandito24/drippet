
#pragma once
#include "bluetooth_task.hpp"
#include "clock.hpp"
#include "constants.hpp"
#include "freertos/idf_additions.h"
#include "head.hpp"
#include "head_status_task.hpp"
#include "head_task.hpp"
#include "logger.hpp"
#include "nvs_storage.hpp"
#include "queue.hpp"
#include "self_node_task.hpp"
#include "task.hpp"
#include <memory>
#include <optional>

using NodeTask_t = std::unique_ptr<SelfNodeTask>;
class HeadEntrypointTask : public Task {

public:
  HeadEntrypointTask(NodeTask_t selfNodeTask, QueueManager qManager)
      : Task("HEAD_ENTRYPOINT", 4096, 2), node_task{std::move(selfNodeTask)},
        q_manager{qManager} {};

  void init() {

    this->storage.init();
    ESP_ERROR_CHECK(this->ble_task.init_stack());
    this->ble_task.start();
    this->status_task.start();
    head_task = std::make_unique<HeadTask>(
        head_node, q_manager.get_handle(QueueOrder::EXT_INCOMING),
        q_manager.get_handle(QueueOrder::EXT_OUTGOING),
        ble_task.get_cccd_subtask_handle());
    head_task->start();
  }

protected:
  void run() override;

private:
  void reset_node();
  NodeTask_t node_task;
  QueueManager q_manager;
  NvsStorage storage{};
  SystemClock sys_clock{};
  Esp32Clock clock{DEV_PHASE_LEN, sys_clock};
  Head head_node{clock, storage, [this] { this->reset_node(); }};
  BluetoothTask ble_task{head_node};
  std::unique_ptr<HeadTask> head_task = nullptr;
  HeadStatusTask status_task{head_node.get_head_status(), STATUS_LED};
};
