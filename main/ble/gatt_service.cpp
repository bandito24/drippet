#include "gatt_service.hpp"
#include "gatt_attribute.hpp"
#include "host/ble_gatt.h"
#include "os/os_mbuf.h"
#include "util.hpp"

Esp_Err_t GattService::init() {

  this->gatt_chr_defs[0] = {
      .uuid = &duration_chr_uuid.u,
      .access_cb = handle_water_durations,
      .arg = &this->attr,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
      .val_handle = &duration_chr_handle,
  };
  this->gatt_chr_defs[1] = {};

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
  Esp_Err_t rc;
  auto attr = static_cast<GattAttribute *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {

    int rc = os_mbuf_append(ctxt->om, attr->duration_buffer.data(),
                            BLE::DURATION_BUFF_LEN);
    // for(uint8_t val : self->duration_buffer){
    //   Logger::log_error( "val is: %d", val);
    // }
    if (rc != 0) {
      Logger::log_error("Could not write data into mbuf");
    }
    return rc;
  }
  case BLE_GATT_ACCESS_OP_WRITE_CHR: {
    uint8_t raw_data[BLE::MAX_PKT_LEN]{};
    if (ctxt->om->om_len) {
      uint8_t header[2];
      rc = os_mbuf_copydata(ctxt->om, 0, BLE::HEADER_LEN, header);
      if (rc) {
        Logger::log_error("Could not allocate enough data for BLE header");
        return rc;
      }
      size_t needed_len = BLE::HEADER_LEN + header[BLE::DATA_LEN_IDX];
      rc = os_mbuf_copydata(ctxt->om, 0, needed_len, raw_data);

      if (rc) {
        Logger::log_error("Could not allocate enough data for BLE data");
        return rc;
      }

      attr->handle_incoming_write(raw_data);

    } else {
      Logger::log_error("Received empty write os_mbuf");
      return 1;
    }

    return 0;
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
