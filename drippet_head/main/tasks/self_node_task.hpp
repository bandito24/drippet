
#pragma once
#include "freertos/idf_additions.h"
#include "node_status_task.hpp"
#include "self_node.hpp"
#include "switch.hpp"
#include "task.hpp"
#include <memory>

class SelfNodeTask : public Task {

public:
  SelfNodeTask(QueueHandle_t incomingQueueHandle,
               QueueHandle_t outgoingQueueHandle)
      : Task("SELF_NODE", 4096, 2), incoming_queue(incomingQueueHandle),
        outgoing_queue(outgoingQueueHandle){};

protected:
  void run() override;

private:
  QueueHandle_t incoming_queue;
  SteadyEspClock steady_clock{};
  QueueHandle_t outgoing_queue;
  std::unique_ptr<SolenoidManager> solenoid_manager;
  std::unique_ptr<SelfNode> self_node;
  std::unique_ptr<NodeStatusTask> led_status_indication;
};
