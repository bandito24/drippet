#include "gap_manager.hpp"
#include "constants.hpp"
#include "gatt_service.hpp"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

bool GapManager::has_conn_handle = false;

Esp_Err_t GapManager::init() {
  Esp_Err_t rc;
  ble_svc_gap_init();
  /* Set GAP device name */
  rc = ble_svc_gap_device_name_set(BLE::DEVICE_NAME.c_str());
  if (rc != 0) {
    Logger::log_simple("failed to set device name to %s, error code: %d",
                       BLE::DEVICE_NAME.c_str(), rc);
  }
  return rc;
}

void GapManager::on_stack_sync() {

  GapManager &gap_manager = GapManager::get_instance();

  int rc = 0;
  char addr_str[18] = {0};

  rc = ble_hs_util_ensure_addr(0);
  if (rc != 0) {
    Logger::log_error("device does not have any available bt address!");
    return;
  }
  rc = ble_hs_id_infer_auto(0, &gap_manager.own_addr_type);
  if (rc != 0) {
    Logger::log_error("failed to infer address type, error code: %d", rc);
    return;
  }

  rc = ble_hs_id_copy_addr(gap_manager.own_addr_type,
                           gap_manager.self_addr.data(), NULL);
  if (rc != 0) {
    Logger::log_error("failed to copy device address, error code: %d", rc);
    return;
  }
  Logger::log_simple("device address: %s", addr_str);
  GapManager::start_advertising();
}
void GapManager::start_advertising() {

  GapManager &gap_manager = GapManager::get_instance();

  const char *name;
  ble_hs_adv_fields adv_field = {};
  ble_hs_adv_fields rsp_fields = {};
  ble_gap_adv_params adv_params = {};
  int rc = 0;

  name = ble_svc_gap_device_name();
  adv_field.name = (uint8_t *)name;
  adv_field.name_len = strlen(name);
  adv_field.name_is_complete = 1;

  adv_field.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
  adv_field.appearance_is_present = 1;
  adv_field.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
  adv_field.le_role_is_present = 1;

  rc = ble_gap_adv_set_fields(&adv_field);
  if (rc != 0) {
    Logger::log_error("failed to set advertising data, error code: %d", rc);
    return;
  }

  rsp_fields.device_addr = gap_manager.self_addr.data();
  rsp_fields.device_addr_type = gap_manager.own_addr_type;
  rsp_fields.device_addr_is_present = 1;

  /* Set advertising interval */
  rsp_fields.adv_itvl = BLE_GAP_ADV_ITVL_MS(500);
  rsp_fields.adv_itvl_is_present = 1;

  /* Set scan response fields */
  rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
  if (rc != 0) {
    Logger::log_error("failed to set scan response data, error code: %d", rc);
    return;
  }

  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  /* Set advertising interval */
  adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(500);
  adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(510);

  /* Start advertising */
  rc = ble_gap_adv_start(gap_manager.own_addr_type, NULL, BLE_HS_FOREVER,
                         &adv_params, GapManager::gap_event_handler,
                         &gap_manager);
  if (rc != 0) {
    Logger::log_error("failed to start advertising, error code: %d", rc);
    return;
  }
  Logger::log_simple("advertising started!");
}

int GapManager::gap_event_handler(ble_gap_event *event, void *arg) {

  auto gap_manager = static_cast<GapManager *>(arg);

  /* Local variables */
  int rc = 0;
  struct ble_gap_conn_desc desc;

  /* Handle different GAP event */
  switch (event->type) {

  /* Connect event */
  case BLE_GAP_EVENT_CONNECT:
    /* A new connection was established or a connection attempt failed. */
    Logger::log_simple("connection %s; status=%d",
                       event->connect.status == 0 ? "established" : "failed",
                       event->connect.status);

    /* Connection succeeded */
    if (event->connect.status == 0) {

      /* Check connection handle */
      rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
      if (rc != 0) {
        Logger::log_error("failed to find connection by handle, error code: %d",
                          rc);
        return rc;
      }

      /* Try to update connection parameters */
      struct ble_gap_upd_params params = {.itvl_min = desc.conn_itvl,
                                          .itvl_max = desc.conn_itvl,
                                          .latency = 0,
                                          .supervision_timeout =
                                              desc.supervision_timeout = 80};
      rc = ble_gap_update_params(event->connect.conn_handle, &params);
      if (rc != 0) {
        Logger::log_error(
            "failed to update connection parameters, error code: %d", rc);
        return rc;
      }
    }
    /* Connection failed, restart advertising */
    else {
      start_advertising();
    }
    return rc;

  /* Disconnect event */
  case BLE_GAP_EVENT_DISCONNECT:
    /* A connection was terminated, print connection descriptor */
    Logger::log_simple("disconnected from peer; reason=%d",
                       event->disconnect.reason);

    /* Restart advertising */
    start_advertising();
    return rc;

  /* Connection parameters update event */
  case BLE_GAP_EVENT_CONN_UPDATE:
    /* The central has updated the connection parameters. */
    Logger::log_simple("connection updated; status=%d",
                       event->conn_update.status);

    /* Print connection descriptor */
    rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
    if (rc != 0) {
      Logger::log_error("failed to find connection by handle, error code: %d",
                        rc);
      return rc;
    }
    return rc;

  /* Advertising complete event */
  case BLE_GAP_EVENT_ADV_COMPLETE:
    /* Advertising completed, restart advertising */
    Logger::log_simple("advertise complete; reason=%d",
                       event->adv_complete.reason);
    start_advertising();
    return rc;

  /* Notification sent event */
  case BLE_GAP_EVENT_NOTIFY_TX:
    if ((event->notify_tx.status != 0) &&
        (event->notify_tx.status != BLE_HS_EDONE)) {
      /* Print notification info on error */
      Logger::log_simple("notify event; conn_handle=%d attr_handle=%d "
                         "status=%d is_indication=%d",
                         event->notify_tx.conn_handle,
                         event->notify_tx.attr_handle, event->notify_tx.status,
                         event->notify_tx.indication);
    }
    return rc;

  /* Subscribe event */
  case BLE_GAP_EVENT_SUBSCRIBE: {
    //    /* Print subscription info to log */
    Logger::log_simple(
        "subscribe event; conn_handle=%d attr_handle=%d "
        "reason=%d prevn=%d curn=%d previ=%d curi=%d",
        event->subscribe.conn_handle, event->subscribe.attr_handle,
        event->subscribe.reason, event->subscribe.prev_notify,
        event->subscribe.cur_notify, event->subscribe.prev_indicate,
        event->subscribe.cur_indicate);

    //    /* GATT subscribe event callback */
    //    gatt_svr_subscribe_cb(event);
    ConnContext &ctxt = gap_manager->conn_context;
    ctxt.conn_handle = event->subscribe.conn_handle;
    ctxt.indicate_status = event->subscribe.cur_indicate;

    return rc;
  }

  /* MTU update event */
  case BLE_GAP_EVENT_MTU:
    /* Print MTU update info to log */
    Logger::log_simple("mtu update event; conn_handle=%d cid=%d mtu=%d",
                       event->mtu.conn_handle, event->mtu.channel_id,
                       event->mtu.value);
    return rc;
  }

  return rc;
}
void GapManager::on_stack_reset(int reason) {
  Logger::log_simple("nimble stack reset, reset reason: %d", reason);
}
