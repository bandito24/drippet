#pragma once
#include "constants.hpp"
#include "gap_manager.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include <stdint.h>

class SecondaryAttr {

public:
  SecondaryAttr(Head &head, const ConnContext &ctxt)
      : head_node(head), conn_ctxt(ctxt){};
  Head &head_node;
  uint16_t chr_handle;
  const ConnContext &conn_ctxt;
};
class NodeDescAttr : public SecondaryAttr {
public:
  using SecondaryAttr::SecondaryAttr;
};
class SysConfigAttr : public SecondaryAttr {
public:
  using SecondaryAttr::SecondaryAttr;
};

class GattService {
public:
  Esp_Err_t init();
  static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt,
                                   void *arg);
  GattAttribute &attr;
  NodeDescAttr &desc_attr;
  GattService(GattAttribute &attribute, NodeDescAttr &desc)
      : attr{attribute}, desc_attr(desc){};

private:
  static int handle_water_durations(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);
  static int handle_read_node_status(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt,
                                     void *arg);
  ble_gatt_chr_def gatt_chr_defs[3];
  ble_gatt_svc_def gatt_svc_table[3];
  ble_uuid128_t durations_svc_uuid =
      BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde,
                       0xef, 0x12, 0x12, 0x25, 0x00, 0x02, 0x00);
  ble_uuid128_t duration_chr_uuid =
      BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde,
                       0xef, 0x12, 0x12, 0x25, 0x00, 0x00, 0x00);

  ble_uuid128_t node_status_chr_uuid =

      BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde,
                       0xef, 0x12, 0x12, 0x25, 0x00, 0x00, 0x01);

  ble_uuid128_t sys_conf_chr_uuid =
      BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde,
                       0xef, 0x12, 0x12, 0x25, 0x00, 0x00, 0x02);
};
