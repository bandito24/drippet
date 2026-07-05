#include "gatt_service.hpp"
#include "ble_serializer.hpp"
#include "clock.hpp"
#include "constants.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "host/ble_gatt.h"
#include "logger.hpp"
#include "os/os_mbuf.h"
#include "secondary_attribute.hpp"
#include "util.hpp"

Esp_Err_t GattService::init() {

  this->gatt_chr_defs[0] = {
      .uuid = &duration_chr_uuid.u,
      .access_cb = handle_water_durations,
      .arg = &this->attr,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
      .val_handle = &this->attr.duration_chr_handle,
  };
  this->gatt_chr_defs[1] = {
      .uuid = &node_status_chr_uuid.u,
      .access_cb = handle_read_node_status,
      .arg = &this->desc_attr,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
      .val_handle = &this->desc_attr.chr_handle,
  };
  // TODO: define and make this
  this->gatt_chr_defs[2] = {
      .uuid = &sys_conf_chr_uuid.u,
      .access_cb = handle_conf_op,
      .arg = &this->conf_attr,
      .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
      .val_handle = &this->conf_attr.chr_handle,
  };

  this->gatt_chr_defs[3] = {
      .uuid = &ext_response_chr_uuid.u,
      .access_cb = handle_ext_response,
      .arg = &this->rsp_attr,
      .flags = BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &this->rsp_attr.chr_handle,
  };
  this->gatt_chr_defs[4] = {};

  this->gatt_svc_table[0] = {
      .type = BLE_GATT_SVC_TYPE_PRIMARY,
      .uuid = &durations_svc_uuid.u,
      .characteristics = gatt_chr_defs,
  };
  this->gatt_svc_table[1] = {};

  this->attr.load_duration_buffer(0);

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
  auto attr = static_cast<GattAttribute *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {

    int rc = os_mbuf_append(ctxt->om, attr->duration_buffer.data(),
                            attr->duration_buffer.size());
    if (rc != 0) {
      Logger::log_error("Could not write data into mbuf");
    }
    return rc;
  }
  case BLE_GATT_ACCESS_OP_WRITE_CHR: {

    BleReadRes res = GattService::read_incoming_data(ctxt);
    if (res.rc == OK_ESP) {
      attr->handle_incoming_write(res.raw_data);
    }
    return res.rc;
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
  auto desc_attr = static_cast<NodeDescAttr *>(arg);

  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {
    all_node_status_t nodes_state = desc_attr->head_node.get_node_statuses();
    BlePacket pkt = BleSerializer::write_node_statuses(nodes_state);
    int rc = os_mbuf_append(ctxt->om, pkt.data(), pkt.size());
    if (rc != 0) {
      Logger::log_error("Could not write data into mbuf");
    }
    return rc;
  }

  default:

    Logger::log_error("Unresolved op event of %d", ctxt->op);
    return 1;
  }
}
BleReadRes GattService::read_incoming_data(struct ble_gatt_access_ctxt *ctxt) {

  BleReadRes read_res{};
  Esp_Err_t &rc = read_res.rc;
  IncomingBLE &raw_data = read_res.raw_data;

  if (ctxt->om->om_len) {
    uint8_t header[2];
    rc = os_mbuf_copydata(ctxt->om, 0, BLE::HEADER_LEN, header);
    if (rc) {
      Logger::log_error("Could not allocate enough data for BLE header");
    }
    size_t needed_len = BLE::HEADER_LEN + header[BLE::DATA_LEN_IDX];
    rc = os_mbuf_copydata(ctxt->om, 0, needed_len, raw_data.data());

    if (rc) {
      Logger::log_error("Could not allocate enough data for BLE data");
    }
  } else {
    Logger::log_error("Received empty write os_mbuf");
  }
  return read_res;
}

int GattService::handle_conf_op(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg) {

  auto attr = static_cast<SysConfigAttr *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_WRITE_CHR: {
    BleReadRes res = GattService::read_incoming_data(ctxt);
    if (res.rc == OK_ESP) {
      attr->handle_ext_write_conf(res.raw_data);
    }
    return res.rc;
  }
  case BLE_GATT_ACCESS_OP_READ_CHR: {
    HourMin curr_time = attr->head_node.get_hourmin_curr_time();

    std::optional<HourMin> next_phase = attr->head_node.get_hourmin_curr_time();
    std::array<uint8_t, 4> cfg{curr_time.hour, curr_time.minute};
    uint8_t len = 2;

    if (next_phase) {
      len = 4;
      cfg[2] = next_phase->hour;
      cfg[3] = next_phase->minute;
    }
    BlePacket pkt = BleSerializer::write_clock_and_phase(cfg, len);

    int rc = os_mbuf_append(ctxt->om, pkt.data(), pkt.size());
    if (rc != 0) {
      Logger::log_error("Could not write data into mbuf");
    }
    return rc;
  }
  default:
    Logger::log_error("Unresolved op event of %d", ctxt->op);
    return 1;
  }
}

int GattService::handle_ext_response(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt,
                                     void *arg) {

  auto attr = static_cast<ExtReqResponseAttr *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {
    OptionalRequest rsp = attr->head_node.extRequestsManager.popEvent();
    if (!rsp) {
      Logger::log_error("No read available on external response");
      return 1;
    }
    int rc = os_mbuf_append(ctxt->om, rsp->data.data(), rsp->data.size());
    if (rc != 0) {
      Logger::log_error("Could not write data into mbuf");
    }
    return rc;
  }
  default:
    Logger::log_error("Unresolved op event of %d", ctxt->op);
    return 1;
  }
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
