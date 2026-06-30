
#include "head_status_task.hpp"
#include "constants.hpp"
#include "head.hpp"
#include "logger.hpp"
#include "protocol_types.hpp"

void HeadStatusTask::run() {
  this->led_indication.init();
  for (;;) {

    switch (this->head_status) {
    case HeadStatus::PAIRING:
      this->led_indication.toggle();
      break;
    case HeadStatus::FAULTY_NODE:
      this->led_indication.disable();
      break;
    default:
      this->led_indication.enable();
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
