#include "freertos/idf_additions.h"

class Queue {
public:
  Queue(UBaseType_t queueLength, UBaseType_t itemSize)
      : uxQueueLength(queueLength), uxItemSize(itemSize) {
    queueHandle = nullptr;
  }
  void init() {
    this->queueHandle = xQueueCreate(uxQueueLength, uxItemSize);
    assert(this->queueHandle != nullptr);
  }
  QueueHandle_t get_handle() const { return queueHandle; }

private:
  QueueHandle_t queueHandle;
  UBaseType_t uxQueueLength;
  UBaseType_t uxItemSize;
};
