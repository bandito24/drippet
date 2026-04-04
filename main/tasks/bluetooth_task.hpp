#include "gap_manager.hpp"
#include "gatt_service.hpp"

#include "constants.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "task.hpp"
class BluetoothTask : public Task {

public:
  BluetoothTask(GattAttribute &gatt_att)
      : Task("BLE", 4096, 4), gap_manager{}, gatt_svc(gatt_att){};
  Esp_Err_t init_stack();

protected:
  void run() override;

private:
  bool stack_initialized = false;
  GapManager gap_manager;
  GattService gatt_svc;
};
