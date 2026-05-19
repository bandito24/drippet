#pragma once
#include "esp32config.hpp"
#include "esp_button.hpp"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "head.hpp"
#include "task.hpp"

constexpr int STOP_PAIRING_COUNT = 10;

class HeadTask : public Task {

public:
  HeadTask(Head &head, QueueHandle_t incomingQueueHandle,
           QueueHandle_t outgoingQueueHandle, TaskHandle_t cccd_subtask)
      : Task("HEAD", 4096, 2), incoming_queue(incomingQueueHandle),
        outgoing_queue(outgoingQueueHandle), headNode(head),
        cccd_task_handle(cccd_subtask),
        start_pairing_btn(PAIR_INIT_GPIO_BTN, [this]() {
          this->no_response_count = 0;
          this->headNode.init_pairing_mode();
        }) {
    ESP_ERROR_CHECK(this->start_pairing_btn.init());
  }

protected:
  void run() override;

private:
  QueueHandle_t incoming_queue;
  QueueHandle_t outgoing_queue;
  Head &headNode;
  int no_response_count = 0;
  TaskHandle_t cccd_task_handle;
  EspButton start_pairing_btn;
};
