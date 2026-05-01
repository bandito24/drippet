#include "head.hpp"
#include "task.hpp"
class WateringTask : public Task {
public:
  WateringTask(Head &head) : Task("WATERING_TASK", 4096, 2), headNode{head} {};
  Head &headNode;

protected:
  void run() override;
};
