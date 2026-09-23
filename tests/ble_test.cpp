#include "ble_link_interface.hpp"
#include "ble_types.hpp"
#include "clock.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "fixtures.hpp"
#include "head.hpp"
#include "logger.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include "protocol_types.hpp"
#include "util.hpp"

#include "type_cast.hpp"
#include <array>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fakeit.hpp>
#include <optional>
#include <span>

TEST_CASE("BLE_Link_interface", "[ble_link]") {

  SECTION("duration schedule can be converted to bytes and vice versa") {
    NodeTypes::DurationSchedule schedule{
        .duration = 500,
        .cycle = {true, true, true, false, false, false, true}};
    FlatNodeSchedule dur_bytes =
        BLELinkInterface::duration_schedule_to_bytes(schedule);
    auto re_schedule = BLELinkInterface::bytes_to_duration_schedule(dur_bytes);

    REQUIRE(schedule.cycle == re_schedule.cycle);
  }

  //  // NOTE: we subtract one because node change req type is called by head
  //  task.
  //  // also dont call init so - 2
  size_t total_req_count = Util::to_i(BLE::Cmds::REQUEST_COUNT);
  using Cmd = BLE::Cmds;

  HeadFixture fix;
  BLELinkInterface ble_if{*fix.head};
  ExtRqManager &mgr = fix.head->extRequestsManager;

  SECTION("All Request peeks are nullopt in beginning") {
    REQUIRE(mgr.peek_event_count() == 0);
    REQUIRE(mgr.peek_request_count() == 0);
  }
  Mocks::populate_head_nodes(*fix.head, 5);

  size_t ready_count = fix.head->get_node_status_count(NodeStatus::READY);
  REQUIRE(ready_count == 5);

  SECTION("Will push and pop from the stack correctly") {
    auto set_clock_bytes =
        BleMocks::make_incoming_cmd(BLE::Cmds::WRITE_CONF_TIME);
    ble_if.handle_writes(set_clock_bytes);
    //
    REQUIRE(mgr.peek_request_count() == 1);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_CONF_TIME) != std::nullopt);

    auto set_phase_bytes =
        BleMocks::make_incoming_cmd(BLE::Cmds::WRITE_CONF_PHASE);
    ble_if.handle_writes(set_phase_bytes);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_CONF_PHASE) != std::nullopt);

    auto set_phase_time_bytes =
        BleMocks::make_incoming_cmd(BLE::Cmds::WRITE_CONF_TIME_PHASE);
    ble_if.handle_writes(set_phase_time_bytes);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_CONF_TIME_PHASE) != std::nullopt);

    auto node_cycle_bytes =
        BleMocks::make_incoming_cmd(BLE::Cmds::WRITE_NODE_CYCLE);
    ble_if.handle_writes(node_cycle_bytes);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_NODE_CYCLE) != std::nullopt);

    auto node_duration_bytes =
        BleMocks::make_incoming_cmd(BLE::Cmds::WRITE_NODE_DURATION);
    ble_if.handle_writes(node_duration_bytes);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_NODE_DURATION) != std::nullopt);

    size_t op_count = fix.head->process_external_requests();
    REQUIRE(op_count == total_req_count -
                            1); // - 1  because havent processing pairing yet,
                                // it refreshes the nodes so cant test behavior
    REQUIRE(mgr.peek_request_count() == 0);
    REQUIRE(mgr.peek_event_count() == total_req_count - 1);

    // NOTE: These are checked in the order they were enqueued
    //
    // Conf Time
    Verify(Method(fix.clockMock, set_time)
               .Using(set_clock_bytes[1], set_clock_bytes[2]))
        .Exactly(1);
    // Conf Phase
    REQUIRE(fix.head->get_phase_of_cycle() ==
            static_cast<CyclePhase>(set_phase_bytes[1]));
    // Conf Phase Time
    Verify(Method(fix.clockMock, set_next_phase_start_time)
               .Using(set_phase_time_bytes[1], set_phase_time_bytes[2]))
        .Exactly(1);

    // Node Duration
    uint16_t node_duration = Util::get_le16(node_duration_bytes.data() + 2);
    uint8_t bytes_addr = node_duration_bytes[1];
    NodeTypes::DurationSchedule duration =
        *(fix.head->get_node_duration_schedule(bytes_addr));
    REQUIRE(duration.duration == node_duration);

    // Node Cycle
    NodeTypes::WateringCycle conf_cycle =
        Util::byte_to_water_cycle(node_cycle_bytes[2]);
    REQUIRE(duration.cycle == conf_cycle);

    // Pairing
    auto pairing_bytes = BleMocks::make_incoming_cmd(BLE::Cmds::INIT_PAIRING);
    ble_if.handle_writes(pairing_bytes);
    REQUIRE(mgr.peek_request(BLE::Cmds::INIT_PAIRING) != std::nullopt);
    fix.head->process_external_requests();
    REQUIRE(mgr.peek_request(BLE::Cmds::INIT_PAIRING) == std::nullopt);
    size_t ready_count = fix.head->get_node_status_count(NodeStatus::READY);
    REQUIRE(fix.head->get_head_status() == HeadStatus::PAIRING);
    REQUIRE(ready_count == 0);

    REQUIRE(mgr.peek_event_count() == total_req_count);

    SECTION("READ_EVENTS read command will correctly pop events off "
            "extRequestsManager") {
      for (size_t i = 0; i < T::to_int(Cmd::REQUEST_COUNT); i++) {
        const SerializedPacketBuffer buffer =
            ble_if.handle_reads(BLE::Read_T::READ_EVENTS);
        BLE::Cmds cmd = static_cast<BLE::Cmds>(buffer.data[0]);
        int count = 0;
        REQUIRE(buffer.len != 0);
        if (buffer.len == 0) {
          Logger::log_error("FAILING ON %d", buffer.data[0]);
        }
        switch (cmd) {
        case Cmd::WRITE_NODE_DURATION:
          count++;
          REQUIRE(buffer.data[2] == bytes_addr);
          REQUIRE(buffer.data[1] == ActionStatus::OK);
        case Cmd::WRITE_NODE_CYCLE:
          count++;
          REQUIRE(buffer.data[2] == bytes_addr);
          REQUIRE(buffer.data[1] == ActionStatus::OK);
        case Cmd::INIT_PAIRING:
          count++;
          break;
        case Cmd::WRITE_CONF_TIME_PHASE:
          count++;
          REQUIRE(buffer.data[1] == ActionStatus::OK);
          break;
        case Cmd::WRITE_CONF_PHASE:
          count++;
          REQUIRE(buffer.data[1] == ActionStatus::OK);
          break;

        case Cmd::WRITE_CONF_TIME:
          count++;
          REQUIRE(buffer.data[1] == ActionStatus::OK);
          break;

        default:
          Logger::log_error("unhandled test cmd value!");
        }
      }
    }

    const SerializedPacketBuffer buffer =
        ble_if.handle_reads(BLE::Read_T::READ_EVENTS);
    REQUIRE(buffer.len == 0);
  }
  SECTION("Will reflect failure in pop requests for invalid node commands") {

    auto node_cycle_bytes =
        BleMocks::make_incoming_cmd(BLE::Cmds::WRITE_NODE_CYCLE);
    node_cycle_bytes[BLE::TGT_ROW_IDX] = 30;
    ble_if.handle_writes(node_cycle_bytes);

    auto node_duration_bytes =
        BleMocks::make_incoming_cmd(BLE::Cmds::WRITE_NODE_DURATION);
    node_duration_bytes[BLE::TGT_ROW_IDX] = 30;
    ble_if.handle_writes(node_duration_bytes);

    fix.head->process_external_requests();

    auto first_fail = ble_if.handle_reads(BLE::Read_T::READ_EVENTS);
    auto second_fail = ble_if.handle_reads(BLE::Read_T::READ_EVENTS);

    Logger::log_simple("first is command %d and second is %d",
                       first_fail.data[0], second_fail.data[0]);
    REQUIRE(first_fail.data[1] == ActionStatus::INVALID_NODE);
    REQUIRE(second_fail.data[1] == ActionStatus::INVALID_NODE);
  }
  SECTION("Will correctly load all node durations into buffer") {
    auto buffer = ble_if.handle_reads(BLE::Read_T::READ_NODE_DURATIONS);
    int i = 0;
    int index = 0;

    while (i < buffer.len) {
      auto duration = Util::get_le16(&buffer.data[i]);
      i += 2;
      auto cycle = Util::byte_to_water_cycle(i++);
      REQUIRE(duration == fix.head->get_node_hose_duration(index));
      index += 1;
    }
    REQUIRE(index == fix.head->get_discovered_node_count());
  }
  SECTION("Will read node statuses") {

    // NOTE: Five is the count that fix.head had initialized nodes in the
    // beginning
    auto buffer = ble_if.handle_reads(BLE::Read_T::READ_NODE_STATES);
    REQUIRE(buffer.len == 5);
    bool valid = true;
    for (int i = 0; i < 5; i++) {
      if (buffer.data[i] != static_cast<uint8_t>(NodeStatus::READY)) {
        valid = false;
      }
    }
    REQUIRE(valid == true);
  }
  SECTION("Will correctly load configuration data") {

    When(Method(fix.clockMock, get_hourmin_curr_time))
        .AlwaysReturn(HourMin{23, 5});

    fix.clockMock.get().set_phase_of_cycle(CyclePhase::SECOND);
    SECTION("With null next watering") {

      When(Method(fix.clockMock, get_hourmin_next_phase)).Return(std::nullopt);

      auto buffer = ble_if.handle_reads(BLE::Read_T::READ_CONFIGURATION);
      auto expected =
          SerializedPacketBuffer{.data = {23, 5, 1, 0, 0, 0}, .len = 6};

      REQUIRE(buffer == expected);
    }
    SECTION("With not null next watering") {

      When(Method(fix.clockMock, get_hourmin_next_phase)).Return(HourMin(6, 9));

      auto buffer = ble_if.handle_reads(BLE::Read_T::READ_CONFIGURATION);
      auto expected =
          SerializedPacketBuffer{.data = {23, 5, 1, 1, 6, 9}, .len = 6};

      REQUIRE(buffer == expected);
    }
  }
}

// TEST_CASE("BLE tests", "[ble]") {
//   HeadFixture fix;
//
//   Mocks::populate_head_nodes(*fix.head, 5);
//   NodeTypes::DurationSchedule sch1 = {.duration = 900,
//                                       .cycle{true, false, true}};
//   NodeTypes::DurationSchedule sch2 = {.duration = 1000,
//                                       .cycle{false, true, false}};
//   auto sch1_byte = BLELinkInterface::duration_schedule_to_bytes(sch1);
//   auto sch2_byte = BLELinkInterface::duration_schedule_to_bytes(sch2);
//   Mocks::set_watering_cycle(*fix.head, 0, sch1.cycle);
//   Mocks::set_node_duration(*fix.head, 0, sch1.duration);
//   Mocks::set_watering_cycle(*fix.head, 1, sch2.cycle);
//   Mocks::set_node_duration(*fix.head, 1, sch2.duration);
//   // End Initialization
//
//   SECTION("Testing handle_incoming_write main entrypoint") {
//     SECTION("handles load row operations") {
//       auto load_row1 = BleMocks::pkt_load_row();
//       load_row1[BLE::TGT_ROW_IDX] = 3;
//       BLE::Status res1 = handle_and_process_req(*fix.head, attr, load_row1);
//       check_buffer_duration(fix, attr, 3);
//
//       REQUIRE(res1 == BLE::Status::OP_OK);
//       load_row1[BLE::TGT_ROW_IDX] = 2;
//
//       handle_and_process_req(*fix.head, attr, load_row1);
//       check_buffer_duration(fix, attr, 2);
//       SECTION("will not execute invalid rows") {
//         load_row1[BLE::TGT_ROW_IDX] = config::max_nodes + 1;
//
//         BLE::Status res3 = handle_and_process_req(*fix.head, attr,
//         load_row1); REQUIRE(res3 == BLE::Status::INVALID_NODE);
//       }
//     }
//     SECTION("Handles write cell commands") {
//       auto cell1 = BleMocks::pkt_write_cell();
//       uint16_t val = Util::get_le16(&cell1[BLE::DATA_START_IDX]);
//       auto target_node = cell1[BLE::TGT_ROW_IDX];
//
//       auto durations1 = fix.head->get_node_hose_duration(target_node);
//
//       BLE::Status res = handle_and_process_req(*fix.head, attr, cell1);
//       attr.load_duration_buffer(target_node);
//       // TODO: find a way to be able to test when the request has been
//       processed
//       // to load the duration buffer
//       auto durations2 = fix.head->get_node_hose_duration(target_node);
//       std::cout << "durations 1:" << durations1.value() << "\n";
//
//       std::cout << "durations 2:" << durations2.value() << "\n";
//       REQUIRE(durations1.value() != durations2.value());
//       REQUIRE(val == durations2.value());
//       Logger::log_simple("target node is %d", target_node);
//
//       check_buffer_duration(fix, attr, target_node);
//     }
//     SECTION("invalid commands not processed") {
//
//       auto row_add = BleMocks::pkt_write_cell();
//       row_add[0] = 9;
//
//       BLE::Status res = handle_and_process_req(*fix.head, attr, row_add);
//       REQUIRE(res == BLE::Status::INVALID_CMD);
//     }
//     SECTION("invalid packet lengths not processed") {
//
//       auto cel_add = BleMocks::pkt_write_cell();
//       auto load = BleMocks::pkt_load_row();
//       std::array<std::span<uint8_t>, 2> vals = {cel_add, load};
//       auto buff1 = attr.duration_buffer;
//       bool failed = false;
//       for (auto &pkt : vals) {
//         pkt[BLE::DATA_LEN_IDX] += 1;
//
//         BLE::Status rc = handle_and_process_req(*fix.head, attr, pkt);
//         if (rc != BLE::Status::INVALID_PKT_LEN) {
//           failed = true;
//         }
//       }
//
//       auto buff2 = attr.duration_buffer;
//       REQUIRE(failed == false);
//       REQUIRE(buff1 == buff2);
//     }
//   }
// }
inline std::ostream &operator<<(std::ostream &os,
                                const NodeTypes::DurationSchedule &schedule) {
  os << "DurationSchedule{"
     << "duration=" << schedule.duration << ", cycle=[";

  for (size_t i = 0; i < schedule.cycle.size(); ++i) {
    os << (schedule.cycle[i] ? '1' : '0');
    if (i + 1 != schedule.cycle.size())
      os << ", ";
  }

  os << "]}";

  return os;
}
