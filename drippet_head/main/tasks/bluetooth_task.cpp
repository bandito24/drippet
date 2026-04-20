#include "bluetooth_task.hpp"
#include "constants.hpp"
#include "esp_err.h"
#include "status_task.hpp"

#include "gap_manager.hpp"
#include "nimble/nimble_port.h"
#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

void ble_store_config_init(void);

#ifdef __cplusplus
}
#endif

Esp_Err_t BluetoothTask::init_stack() {
  Esp_Err_t rc;

  rc = nimble_port_init();
  if (rc != ESP_OK) {
    Logger::log_error("failed to initialize nimble stack, error code: %d ", rc);
    return rc;
  }
  rc = this->gap_manager.init();
  if (rc != 0) {
    Logger::log_error("failed to initialize GAP service, error code: %d", rc);
    return rc;
  }
  rc = this->gatt_svc.init();
  if (rc != 0) {
    Logger::log_error("failed to initialize GATT server, error code: %d", rc);
    return rc;
  }

  ble_hs_cfg.reset_cb = gap_manager.on_stack_reset;
  ble_hs_cfg.sync_cb = gap_manager.on_stack_sync;
  ble_hs_cfg.gatts_register_cb = this->gatt_svc.gatt_svr_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_store_config_init();

  this->stack_initialized = true;

  return 0;
}
void BluetoothTask::run() {
  assert(this->stack_initialized == true);
  this->cccd_subtask.start();
  nimble_port_run();
}
