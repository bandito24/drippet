#pragma once
#include "ble_link_interface.hpp"
#include "constants.hpp"
#include "freertos/idf_additions.h"
#include "gap_manager.hpp"
#include "gatt_char.hpp"
#include "gatt_service.hpp"
#include "head.hpp"
#include "status_task.hpp"
#include "task.hpp"
class BluetoothTask : public Task {

public:
  BluetoothTask(Head &head_node)
      : Task("BLE", 4096, 4), gap_manager{GapManager::get_instance()},
        interface(head_node),
        gatt_svc(this->node_cfg_chr, this->node_desc_chr, this->sys_cfg_char,
                 this->ext_req_chr, this->interface),
        cccd_subtask(this->node_desc_chr, this->ext_req_chr,
                     this->gap_manager.get_ctxt()){};
  Esp_Err_t init_stack();
  TaskHandle_t get_cccd_subtask_handle() const {
    return this->cccd_subtask.get_handle();
  }

protected:
  void run() override;

private:
  GapManager &gap_manager;
  bool stack_initialized = false;
  BLELinkInterface interface;
  NodeConfigChar node_cfg_chr{};
  NodeDescChar node_desc_chr{};
  ExtReqChar ext_req_chr{};
  SysConfigChar sys_cfg_char{};
  GattService gatt_svc;
  StatusTask cccd_subtask;
};
