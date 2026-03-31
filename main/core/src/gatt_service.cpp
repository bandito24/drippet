#include "gatt_service.hpp"
#include "host/ble_gatt.h"
#include "logger.hpp"
#include "os/endian.h"
#include "os/os_mbuf.h"

Esp_Err_t GattService::init() {

  this->gatt_chr_defs[0] = {
      .uuid = &duration_chr_uuid.u,
      .access_cb = handle_water_durations,
      .arg = this,
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

  this->load_duration_buffer(0);

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

void GattService::load_duration_buffer(size_t row) {
  assert(row < config::max_nodes);
  size_t insert_addr = 0;
  for (size_t i = 0; i < config::node_hose_count; i++) {

    put_le16(&this->duration_buffer[insert_addr], this->durations[row][i]);
    insert_addr += 2;
  }
}

int GattService::handle_water_durations(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt,
                                        void *arg) {
  Esp_Err_t rc;
  auto self = static_cast<GattService *>(arg);
  switch (ctxt->op) {
  case BLE_GATT_ACCESS_OP_READ_CHR: {

    int rc =
        os_mbuf_append(ctxt->om, self->duration_buffer, BLE::DURATION_BUFF_LEN);
    if (rc != 0) {
      Logger::log_error("Could not write data into mbuf");
    }
    return rc;
  }
  case BLE_GATT_ACCESS_OP_WRITE_CHR: {
    uint8_t raw_data[BLE::MAX_LEN]{};
    if (ctxt->om->om_len) {
      uint8_t header[2];
      rc = os_mbuf_copydata(ctxt->om, 0, 2, header);
      if (rc) {
        Logger::log_error("Could not allocate enough data for BLE header");
        return rc;
      }
      rc = os_mbuf_copydata(ctxt->om, 0,
                            header[static_cast<size_t>(BLE::Header::DATA_LEN)],
                            raw_data);

      if (rc) {
        Logger::log_error("Could not allocate enough data for BLE data");
        return rc;
      }

      self->handle_incoming_write(raw_data);

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
void GattService::handle_incoming_write(std::span<uint8_t> raw_data) {
  BLE::Cmds CMD = static_cast<BLE::Cmds>(raw_data[0]);
  size_t target_row = raw_data[BLE::TGT_ROW_IDX];
  if (target_row >= config::max_nodes) {
    Logger::log_error("Invalid row passed in of %d", target_row);
  }

  switch (CMD) {
  case BLE::Cmds::LOAD_ROW: {
    this->load_duration_buffer(target_row);
    break;
  }
  case BLE::Cmds::WRITE_CELL: {
    size_t target_col = raw_data[BLE::TGT_ROW_IDX + 1];
    uint16_t new_duration = get_le16(&raw_data[BLE::TGT_ROW_IDX + 2]);
    set_node_duration(target_row, target_col, new_duration);
    this->load_duration_buffer(target_row);
    break;
  }
  case BLE::Cmds::WRITE_ROW: {
    size_t start_idx = BLE::TGT_ROW_IDX + 1;
    for (size_t target_col = 0; target_col < config::node_hose_count;
         target_col++) {
      uint16_t new_duration = get_le16(&raw_data[start_idx]);
      set_node_duration(target_row, target_col, new_duration);
      start_idx += 2;
    }
    this->load_duration_buffer(target_row);
    break;
  }
  default:
    return;
  };
}
void GattService::set_node_duration(size_t node, size_t hose,
                                    uint16_t duration) {
  assert(node <= config::max_nodes);
  assert(hose <= config::node_hose_count);
  this->durations[node][hose] = duration;
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
