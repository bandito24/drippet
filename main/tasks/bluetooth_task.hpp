#include "gap_manager.hpp"
#include "gatt_service.hpp"

#include "constants.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "task.hpp"
class BluetoothTask : public Task {

public:
  BluetoothTask(Head &head_node)
      : Task("BLE", 4096, 4), gatt_attr{head_node},
        node_desc_attr{this->conn_context},
        gap_manager{GapManager::get_instance(conn_context)},
        gatt_svc(this->gatt_attr, this->node_desc_attr){};
  Esp_Err_t init_stack();

protected:
  void run() override;

private:
  ConnContext conn_context{};
  bool stack_initialized = false;
  GattAttribute gatt_attr;
  NodeDescAttr node_desc_attr;
  GapManager &gap_manager;
  GattService gatt_svc;
};
