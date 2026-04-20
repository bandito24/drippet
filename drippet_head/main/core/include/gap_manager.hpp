#pragma once
#include "conn_context.hpp"
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
  static void assign_conn_handle(ConnContext &conn_ctxt);

  static GapManager &get_instance() {
    static GapManager instance{};
    return instance;
  }
  GapManager(GapManager &) = delete;
  GapManager(GapManager &&) = delete;
  void operator=(const GapManager &) = delete;
  const ConnContext &get_ctxt() const { return this->conn_context; }

private:
  static bool has_conn_handle;
  static ConnContext &conn_ctxt;
  static void start_advertising();
  static int gap_event_handler(ble_gap_event *event, void *arg);
  GapManager() = default;

  ConnContext conn_context{};

  std::array<uint8_t, 6> self_addr{0};
  uint8_t own_addr_type{};
};
