#pragma once
#include "freertos/idf_additions.h"
#include "status_task.hpp"

#include "constants.hpp"
#include "gatt_attribute.hpp"
#include "gatt_service.hpp"
#include "head.hpp"
#include "task.hpp"
class BluetoothTask : public Task {

public:
  BluetoothTask(Head &head_node)
      : Task("BLE", 4096, 4), gap_manager{GapManager::get_instance()},
        gatt_attr{head_node},
        node_desc_attr{head_node, this->gap_manager.get_ctxt()},
        gatt_svc(this->gatt_attr, this->node_desc_attr),
        cccd_subtask(this->node_desc_attr, this->gap_manager.get_ctxt()){};
  Esp_Err_t init_stack();
  TaskHandle_t get_cccd_subtask_handle() const {
    return this->cccd_subtask.get_handle();
  }

protected:
  void run() override;

private:
  GapManager &gap_manager;
  bool stack_initialized = false;
  GattAttribute gatt_attr;
  NodeDescAttr node_desc_attr;
  GattService gatt_svc;
  StatusTask cccd_subtask;
};
