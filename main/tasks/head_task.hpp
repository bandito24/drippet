#include "freertos/idf_additions.h"
#include "head.hpp"
#include "node.hpp"
#include "task.hpp"
#include "time.hpp"
class HeadTask : public Task {

public:
  HeadTask(MainValve &valve, iClock &clock, QueueHandle_t incomingQueueHandle,
           QueueHandle_t outgoingQueueHandle)
      : Task("HEAD", 4096, 2), incoming_queue(incomingQueueHandle),
        outgoing_queue(outgoingQueueHandle),

        headNode(valve, clock){};

protected:
  void run() override;

private:
  QueueHandle_t incoming_queue;
  QueueHandle_t outgoing_queue;
  Head headNode;
};
