
#pragma once
#include "freertos/idf_additions.h"
#include "self_node.hpp"
#include "task.hpp"

class NodeTask : public Task {

public:
  NodeTask(SelfNode &selfNode, QueueHandle_t incomingQueueHandle,
           QueueHandle_t outgoingQueueHandle)
      : Task("SELF_NODE", 4096, 2), incoming_queue(incomingQueueHandle),
        outgoing_queue(outgoingQueueHandle), self_node{selfNode} {};

protected:
  void run() override;

private:
  QueueHandle_t incoming_queue;
  QueueHandle_t outgoing_queue;
  SelfNode &self_node;
};
