#include "bluetooth_task.hpp"
#include "esp_err.h"
#include "gatt_service.hpp"
#include "logger.hpp"

#include "gap_manager.hpp"
#include "nimble/nimble_port.h"
#include "nvs_flash.h"

static void nimble_task(void *param) {

  nimble_port_run();
  vTaskDelete(NULL);
}

#ifdef __cplusplus
extern "C" {
#endif

void ble_store_config_init(void);

#ifdef __cplusplus
}
#endif

GapManager gap_manager;
GattService gatt_service;

void bluetooth_init() {
  Esp_Err_t rc;

  rc = nvs_flash_init();
  if (rc == ESP_ERR_NVS_NO_FREE_PAGES || rc == ESP_ERR_NO_MEM) {
    nvs_flash_erase();
    rc = nvs_flash_init();
    ESP_ERROR_CHECK(rc);
  }

  rc = nimble_port_init();
  if (rc != ESP_OK) {
    Logger::log_error("failed to initialize nimble stack, error code: %d ", rc);
    return;
  }
  rc = gap_manager.init();
  if (rc != 0) {
    Logger::log_error("failed to initialize GAP service, error code: %d", rc);
    return;
  }
  rc = gatt_service.init();
  if (rc != 0) {
    Logger::log_error("failed to initialize GATT server, error code: %d", rc);
    return;
  }

  ble_hs_cfg.reset_cb = gap_manager.on_stack_reset;
  ble_hs_cfg.sync_cb = gap_manager.on_stack_sync;
  ble_hs_cfg.gatts_register_cb = gatt_service.gatt_svr_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_store_config_init();

  xTaskCreate(nimble_task, "NIMBLE TASK", 4 * 1024, NULL, 5, NULL);
  return;
}
