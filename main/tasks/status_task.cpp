#include "status_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "head.hpp"
#include "portmacro.h"
void StatusTask::run() {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    Logger::log_simple("Task for notification is running");

    if (this->conn_ctxt.indicate_status) {

      Logger::log_simple("Notification task has passed all checks");
      ble_gatts_indicate(this->conn_ctxt.conn_handle,
                         this->desc_attr.node_status_chr_handle);
    }
  }
}
