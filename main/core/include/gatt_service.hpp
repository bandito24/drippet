#include "config.hpp"
#include "constants.hpp"
#include "gatt_attribute.hpp"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include <cstddef>
#include <span>
#include <stdint.h>

class GattService {
public:
  Esp_Err_t init();

  static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt,
                                   void *arg);
  GattAttribute attr;

private:
  static int handle_water_durations(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);
  ble_gatt_chr_def gatt_chr_defs[2];
  ble_gatt_svc_def gatt_svc_table[2];
  uint16_t duration_chr_handle;
  ble_uuid128_t durations_svc_uuid =
      BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde,
                       0xef, 0x12, 0x12, 0x25, 0x00, 0x02, 0x00);
  ble_uuid128_t duration_chr_uuid =
      BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde,
                       0xef, 0x12, 0x12, 0x25, 0x00, 0x00, 0x00);
};
