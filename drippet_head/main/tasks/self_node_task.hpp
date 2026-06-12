
#pragma once
#include "freertos/idf_additions.h"
#include "node_status_task.hpp"
#include "self_node.hpp"
#include "task.hpp"
#include <memory>

using Qt = QueueHandle_t;
// Dual is for when the device is both head and node
enum class NodeMode { STANDARD, DUAL };

class SelfNodeTask : public Task {

public:
  // Don't pass in queue manager since we want self_node to be unaware of what
  // queue it's using
  SelfNodeTask(QueueHandle_t incomingQueueHandle,
               QueueHandle_t outgoingQueueHandle, NodeMode _mode)
      : Task("SELF_NODE", 4096, 2), incoming_queue(incomingQueueHandle),
        outgoing_queue(outgoingQueueHandle), mode(_mode){};
  // This is a flag for knowing whether the node should switch to head mode
  bool heard_communication = false;
  ~SelfNodeTask() {
    if (this->mode == NodeMode::STANDARD) {
      led_status_indication->led_indication.disable();
      vTaskDelete(led_status_indication->get_handle());
    }
  }

  void disable_led() { led_status_indication->disable_led_indication(); }
  void request_delete_task() { this->delete_task_requested = true; }
  bool is_task_running() { return !task_stopped; }

  // TODO: make these private
  QueueHandle_t incoming_queue;
  QueueHandle_t outgoing_queue;

protected:
  void run() override;

private:
  SteadyEspClock steady_clock{};
  NodeMode mode;
  std::unique_ptr<SelfNode> self_node;
  std::unique_ptr<NodeStatusTask> led_status_indication;

  volatile bool delete_task_requested = false;
  volatile bool task_stopped = false;
};
