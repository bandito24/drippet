#include "status_task.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "head.hpp"
#include "head_task.hpp"
#include "host/ble_gatt.h"
#include "portmacro.h"
void StatusTask::run() {
  for (;;) {
    uint32_t value;
    xTaskNotifyWait(0, UINT32_MAX, &value, portMAX_DELAY);

    if (this->conn_ctxt.indicate_status &&
        (value & static_cast<uint32_t>(EVENT_BITS::NODE_CHANGE))) {
      ble_gatts_indicate(this->conn_ctxt.conn_handle,
                         this->desc_attr.chr_handle);
    }
    if (this->conn_ctxt.notify_status &&
        (value & static_cast<uint32_t>(EVENT_BITS::EXT_RESPONSE))) {
      ble_gatts_notify(this->conn_ctxt.conn_handle, this->rsp_attr.chr_handle);
      // TODO: make associated static function etc
    }
  }
}
