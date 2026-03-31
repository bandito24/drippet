#include "gap_manager.hpp"
#include "gatt_service.hpp"

#include "constants.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "task.hpp"
class BluetoothTask : public Task {

public:
  BluetoothTask(Head &head, GattAttribute &gatt_att)
      : Task("BLE", 4096, 4), head{head}, gap_manager{}, gatt_att{gatt_att},
        gatt_svc(gatt_att){};

protected:
  void run() override;
  Esp_Err_t init_stack();

private:
  bool stack_initialized = false;
  Head &head;
  GapManager gap_manager;
  GattAttribute &gatt_att;
  GattService gatt_svc;
};
