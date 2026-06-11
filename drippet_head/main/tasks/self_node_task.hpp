
#pragma once
#include "freertos/idf_additions.h"
#include "node_status_task.hpp"
#include "self_node.hpp"
#include "task.hpp"
#include <memory>

using Qt = QueueHandle_t;

class SelfNodeTask : public Task {

public:
  SelfNodeTask(QueueHandle_t incomingQueueHandle,
               QueueHandle_t outgoingQueueHandle)
      : Task("SELF_NODE", 4096, 2), incoming_queue(incomingQueueHandle),
        outgoing_queue(outgoingQueueHandle){};
  // This is a flag for knowing whether the node should switch to head mode
  bool heard_communication = false;
  ~SelfNodeTask() {
    led_status_indication->led_indication.disable();
    vTaskDelete(led_status_indication->get_handle());
  }
  void disable_led() { led_status_indication->disable_led_indication(); }
  void change_queues_for_local_node(Qt localIncoming, Qt localOutgoing) {
    this->incoming_queue = localIncoming;
    this->outgoing_queue = localOutgoing;
  }

protected:
  void run() override;

private:
  QueueHandle_t incoming_queue;
  SteadyEspClock steady_clock{};
  QueueHandle_t outgoing_queue;
  std::unique_ptr<SelfNode> self_node;
  std::unique_ptr<NodeStatusTask> led_status_indication;
};
