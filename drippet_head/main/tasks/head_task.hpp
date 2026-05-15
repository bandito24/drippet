#pragma once
#include "freertos/idf_additions.h"
#include "head.hpp"
#include "task.hpp"

constexpr int STOP_PAIRING_COUNT = 7;

class HeadTask : public Task {

public:
  HeadTask(Head &head, QueueHandle_t incomingQueueHandle,
           QueueHandle_t outgoingQueueHandle, TaskHandle_t cccd_subtask)
      : Task("HEAD", 4096, 2), incoming_queue(incomingQueueHandle),
        outgoing_queue(outgoingQueueHandle), headNode(head),
        cccd_task_handle(cccd_subtask) {}

protected:
  void run() override;

private:
  QueueHandle_t incoming_queue;
  QueueHandle_t outgoing_queue;
  Head &headNode;
  int no_response_count = 0;
  TaskHandle_t cccd_task_handle;
};
