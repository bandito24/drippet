#pragma once
#include "ble_link_interface.hpp"
#include "ble_types.hpp"
#include "constants.hpp"
#include "gatt_char.hpp"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include <stdint.h>

using IncomingBLE = std::array<uint8_t, BLE::MAX_INCOMING_PKT_LEN>;
struct BleReadRes {
  Esp_Err_t rc;
  IncomingBLE raw_data;
};
class GattService {
public:
  Esp_Err_t init();
  static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt,
                                   void *arg);
  // NodeConfigChar &attr;
  // NodeDescAttr &desc_attr;

  // SysConfigAttr &conf_attr;
  // ExtReqResponseAttr &rsp_attr;
  NodeConfigChar &node_cfg_chr;
  NodeDescChar &node_desc_chr;
  SysConfigChar &sys_cfg_chr;
  ExtReqChar &ext_req_chr;
  BLELinkInterface &ble_interface;
  GattService(NodeConfigChar &attribute, NodeDescChar &desc,
              SysConfigChar &confAttr, ExtReqChar &rspAttr,
              BLELinkInterface &interface)
      : node_cfg_chr{attribute}, node_desc_chr(desc), sys_cfg_chr(confAttr),
        ext_req_chr{rspAttr}, ble_interface(interface){};

private:
  static int handle_water_durations(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);
  static int handle_read_node_status(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt,
                                     void *arg);

  static int handle_conf_op(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);

  static int handle_ext_response(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg);
  static int handle_service_read(BLE::Read_T read_t,
                                 BLELinkInterface *ble_interface,
                                 struct ble_gatt_access_ctxt *ctxt);
  static int handle_service_writes(BLELinkInterface *ble_interface,
                                   struct ble_gatt_access_ctxt *ctxt);
  static BleReadRes read_incoming_data(struct ble_gatt_access_ctxt *ctxt);

  ble_gatt_chr_def gatt_chr_defs[5];
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

  ble_uuid128_t ext_response_chr_uuid =
      BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde,
                       0xef, 0x12, 0x12, 0x25, 0x00, 0x00, 0x03);
};
