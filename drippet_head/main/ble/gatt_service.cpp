#include "gatt_service.hpp"
#include "logger.hpp"
#include "os/os_mbuf.h"

Esp_Err_t GattService::init() {

  this->gatt_chr_defs[0] = {
      .uuid = &duration_chr_uuid.u,
      .access_cb = handle_water_durations,
      .arg = &this->ble_interface,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
      .val_handle = &this->node_cfg_chr.chr_handle,
  };
  this->gatt_chr_defs[1] = {
      .uuid = &node_status_chr_uuid.u,
      .access_cb = handle_read_node_status,
      .arg = &this->ble_interface,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
      .val_handle = &this->node_desc_chr.chr_handle,
  };
  this->gatt_chr_defs[2] = {
      .uuid = &sys_conf_chr_uuid.u,
      .access_cb = handle_conf_op,
      .arg = &this->ble_interface,
      .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
      .val_handle = &this->sys_cfg_chr.chr_handle,
  };

  this->gatt_chr_defs[3] = {
      .uuid = &ext_response_chr_uuid.u,
      .access_cb = handle_ext_response,
      .arg = &this->ble_interface,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &this->ext_req_chr.chr_handle,
  };
  this->gatt_chr_defs[4] = {};

  this->gatt_svc_table[0] = {
      .type = BLE_GATT_SVC_TYPE_PRIMARY,
      .uuid = &durations_svc_uuid.u,
      .characteristics = gatt_chr_defs,
  };
  this->gatt_svc_table[1] = {};

  Esp_Err_t rc;
  rc = ble_gatts_count_cfg(gatt_svc_table);
  if (rc != 0) {
    Logger::log_simple("Failed counting config services");
    return rc;
  }

  rc = ble_gatts_add_svcs(gatt_svc_table);
  if (rc != 0) {

    Logger::log_simple("Failed adding config services");
    return rc;
  }

  return 0;
}

int GattService::handle_water_durations(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt,
                                        void *arg) {
  auto interface = static_cast<BLELinkInterface *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {

    return GattService::handle_service_read(BLE::Read_T::READ_NODE_DURATIONS,
                                            interface, ctxt);
  }
  case BLE_GATT_ACCESS_OP_WRITE_CHR: {
    return GattService::handle_service_writes(interface, ctxt);
  }
  default:
    Logger::log_error("Unresolved op event of %d", ctxt->op);
    return 1;
  }
}

int GattService::handle_read_node_status(uint16_t conn_handle,
                                         uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt,
                                         void *arg) {

  auto interface = static_cast<BLELinkInterface *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {

    return GattService::handle_service_read(BLE::Read_T::READ_NODE_STATES,
                                            interface, ctxt);
  }

  default:

    Logger::log_error("Unresolved op event of %d", ctxt->op);
    return 1;
  }
}

int GattService::handle_conf_op(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg) {

  auto interface = static_cast<BLELinkInterface *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_WRITE_CHR: {
    return GattService::handle_service_writes(interface, ctxt);
  }
  case BLE_GATT_ACCESS_OP_READ_CHR: {
    return GattService::handle_service_read(BLE::Read_T::READ_CONFIGURATION,
                                            interface, ctxt);
  default:
    Logger::log_error("Unresolved op event of %d", ctxt->op);
    return 1;
  }
  }
}

int GattService::handle_ext_response(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt,
                                     void *arg) {

  auto interface = static_cast<BLELinkInterface *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {
    return GattService::handle_service_read(BLE::Read_T::READ_EVENTS, interface,
                                            ctxt);
  }
  default:
    Logger::log_error("Unresolved op event of %d", ctxt->op);
    return 1;
  }
}

int GattService::handle_service_read(BLE::Read_T read_t,
                                     BLELinkInterface *ble_interface,
                                     struct ble_gatt_access_ctxt *ctxt) {
  const SerializedPacketBuffer buffer = ble_interface->handle_reads(read_t);
  int rc = os_mbuf_append(ctxt->om, buffer.data.data(), buffer.len);
  if (rc != 0) {
    Logger::log_error("Could not write data into mbuf");
  }
  return rc;
}

int GattService::handle_service_writes(BLELinkInterface *ble_interface,
                                       struct ble_gatt_access_ctxt *ctxt) {

  BleReadRes res = GattService::read_incoming_data(ctxt);
  if (res.rc == OK_ESP) {
    ble_interface->handle_writes(res.raw_data);
  }
  return res.rc;
}

BleReadRes GattService::read_incoming_data(struct ble_gatt_access_ctxt *ctxt) {

  BleReadRes read_res{};
  Esp_Err_t &rc = read_res.rc;
  IncomingBLE &raw_data = read_res.raw_data;

  if (ctxt->om->om_len) {
    rc = os_mbuf_copydata(ctxt->om, 0, BLE::MAX_INCOMING_PKT_LEN, raw_data.data());

    if (rc) {
      Logger::log_error("Could not allocate enough data for BLE data");
    }
  } else {
    Logger::log_error("Received empty write os_mbuf");
  }
  return read_res;
}

void GattService::gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt,
                                       void *arg) {
  /* Local variables */
  char buf[BLE_UUID_STR_LEN];

  /* Handle GATT attributes register events */
  switch (ctxt->op) {

  /* Service register event */
  case BLE_GATT_REGISTER_OP_SVC:
    Logger::log_simple("registered service %s with handle=%d",
                       ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                       ctxt->svc.handle);
    break;

  /* Characteristic register event */
  case BLE_GATT_REGISTER_OP_CHR:
    Logger::log_simple("registering characteristic %s with "
                       "def_handle=%d val_handle=%d",
                       ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                       ctxt->chr.def_handle, ctxt->chr.val_handle);
    break;

  /* Descriptor register event */
  case BLE_GATT_REGISTER_OP_DSC:
    Logger::log_simple("registering descriptor %s with handle=%d",
                       ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                       ctxt->dsc.handle);
    break;

  /* Unknown event */
  default:
    assert(0);
    break;
  }
}
