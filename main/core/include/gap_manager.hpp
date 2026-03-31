#pragma once
#include "constants.hpp"
#include "host/ble_gap.h"
#include <array>
constexpr uint16_t BLE_GAP_APPEARANCE_GENERIC_TAG = 0x0200;
constexpr uint16_t BLE_GAP_LE_ROLE_PERIPHERAL = 0x00;

class GapManager {
public:
  Esp_Err_t init();
  static void on_stack_sync();
  static void on_stack_reset(int reason);

private:
  static void start_advertising();
  static int gap_event_handler(ble_gap_event *event, void *arg);

  static std::array<uint8_t, 6> self_addr;
  static uint8_t own_addr_type;
};
