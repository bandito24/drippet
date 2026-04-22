#include "clock.hpp"
#include "constants.hpp"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "node.hpp"
#include "nvs_storage.hpp"
#include "queue.hpp"
#include "tasks/bluetooth_task.hpp"
#include "tasks/head_task.hpp"
#include "uart_task.hpp"

// TESTING ONLY FUNCTIONS
inline NodeTypes::HoseDurations get_new_hose_durations(int increaser);
inline void populate_head_nodes(Head &head, size_t node_count);

extern "C" void app_main(void) {
  setenv("TZ", "UTC", 1);
  tzset();

  UartDriver driver{};
  ESP_ERROR_CHECK(driver.init());
  NvsStorage storage;
  storage.init();
  UartProtocol uart{driver};
  SystemClock sys_clock{};
  Esp32Clock clock{sys_clock};
  MainValve main_valve{};
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

  // For testing only
  // TODO: delete all but vtaskdelay
  populate_head_nodes(head_node, 4);
  int testint = 0;
  while (true) {
    if (testint % 2 == 0) {
      head_node.set_node_status(0, NodeStatus::COMMAND_SENT);
    } else {
      head_node.set_node_status(0, NodeStatus::IN_QUEUE);
    }
    testint += 1;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// TESTING ONLY FUNCTIONS
inline NodeTypes::HoseDurations get_new_hose_durations(int increaser) {

  NodeTypes::HoseDurations hose_durations = {1, 2, 3, 4, 5};
  NodeTypes::HoseDurations ret{};
  for (size_t i = 0; i < hose_durations.size(); i++) {
    ret[i] = hose_durations[i] * (2 + increaser);
  }
  return ret;
}

// TODO: delete this
inline void populate_head_nodes(Head &head,
                                size_t node_count = config::max_nodes) {

  assert(node_count <= config::max_nodes);
  for (size_t addr = 0; addr < node_count; addr++) {
    head.create_node_pending(addr);
    head.confirm_node_pending(addr, addr);
    // auto new_durations = get_new_hose_durations(addr);
    // head.set_node_durations(addr, new_durations);
  }
}
