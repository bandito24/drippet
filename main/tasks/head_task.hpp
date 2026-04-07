#pragma once
#include "freertos/idf_additions.h"
#include "head.hpp"
#include "node.hpp"
#include "task.hpp"
#include <optional>

class HeadTask : public Task {

public:
  HeadTask(Head &head, QueueHandle_t incomingQueueHandle,
           QueueHandle_t outgoingQueueHandle, TaskHandle_t cccd_subtask,
           TaskHandle_t self_handle)
      : Task("HEAD", 4096, 2, self_handle), incoming_queue(incomingQueueHandle),
        outgoing_queue(outgoingQueueHandle), headNode(head),
        cccd_task_handle(cccd_subtask) {}

protected:
  void run() override;

private:
  QueueHandle_t incoming_queue;
  QueueHandle_t outgoing_queue;
  Head &headNode;
  TaskHandle_t cccd_task_handle;
};
