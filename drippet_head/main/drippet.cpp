#include "clock.hpp"
#include "constants.hpp"
#include "esp32config.hpp"
#include "esp_err.h"
#include "esp_switch.hpp"
#include "freertos/idf_additions.h"
#include "head_status_task.hpp"
#include "node.hpp"
#include "nvs_storage.hpp"
#include "queue.hpp"
#include "self_node.hpp"
#include "switch.hpp"
#include "tasks/bluetooth_task.hpp"
#include "tasks/head_task.hpp"
#include "uart_task.hpp"

extern "C" void app_main(void) {
  setenv("TZ", "UTC", 1);
  tzset();

  UartDriver driver{};
  ESP_ERROR_CHECK(driver.init());

  NvsStorage storage;
  storage.init();
  UartProtocol uart{driver};
  SystemClock sys_clock{};
  Esp32Clock clock{65000, sys_clock};
  clock.set_time(2026, 1, 1, 0, 0);

  //// For Queueing Incoming Messages From Peripherals and Sending Head Queued
  //// Messages
  Queue incoming_queue(10, sizeof(UartMessage));
  incoming_queue.init();
  Queue outgoing_queue(10, sizeof(UartMessage));
  outgoing_queue.init();

  UartTask uart_task{uart, outgoing_queue.get_handle(),
                     incoming_queue.get_handle()};
  uart_task.start();
  EspSwitch main_valve{SOLENOID_PIN, GpioActiveLevel::ACTIVE_HIGH};
  Head head_node{main_valve, clock, storage};
  // Starting the bluetooth task and stack
  BluetoothTask ble_task{head_node};
  Esp_Err_t ble_rc = ble_task.init_stack();
  ESP_ERROR_CHECK(ble_rc);
  ble_task.start();

  // Starting the head task that contacts nodes and updates accordingly
  HeadTask head_task{head_node, incoming_queue.get_handle(),
                     outgoing_queue.get_handle(),
                     ble_task.get_cccd_subtask_handle()};
  head_task.start();
  HeadStatusTask status_task{head_node.get_head_status(), STATUS_LED};
  status_task.start();

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
