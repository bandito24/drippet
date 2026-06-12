#pragma once
#include "freertos/idf_additions.h"
#include "protocol.hpp"
#include <array>

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

enum class QueueOrder {
  EXT_OUTGOING,
  EXT_INCOMING,
  INTERNAL_OUTGOING,
  INTERNAL_INCOMING,
  QUEUE_COUNT
};
constexpr size_t queue_count = static_cast<size_t>(QueueOrder::QUEUE_COUNT);

class QueueManager {

public:
  void init() {
    for (auto &queue : this->queues) {
      queue.init();
    }
  }
  QueueHandle_t get_handle(QueueOrder order) {
    return this->queues.at(s_cast(order)).get_handle();
  }

private:
  std::array<Queue, queue_count> queues{
      Queue{10, sizeof(UartMessage)},
      Queue{10, sizeof(UartMessage)},
      Queue{10, sizeof(UartMessage)},
      Queue{10, sizeof(UartMessage)},
  };
  size_t s_cast(QueueOrder order) { return static_cast<size_t>(order); }
};
