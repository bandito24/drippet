#include "config.hpp"
#include "fixtures.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "logger.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include "protocol_types.hpp"
#include "util.hpp"

#include <array>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <span>

// this functino makes sure that the duration buffer matches the provided node
// durations and its indexing
void check_buffer_duration(HeadFixture &fix, GattAttribute &attr,
                           size_t node_index);
constexpr uint8_t test_address = 1;

BLE::Status handle_and_process_req(Head &head, GattAttribute &attr,
                                   std::span<uint8_t> packet) {
  BLE::Status status = attr.handle_incoming_write(packet);
  head.process_external_requests();
  return status;
}

TEST_CASE("BLE tests", "[ble]") {
  HeadFixture fix;

  Mocks::populate_head_nodes(*fix.head, 5);
  NodeTypes::DurationSchedule sch1 = {.duration = 900,
                                      .cycle{true, false, true}};
  NodeTypes::DurationSchedule sch2 = {.duration = 1000,
                                      .cycle{false, true, false}};
  auto sch1_byte = GattAttribute::duration_schedule_to_bytes(sch1, 0);
  auto sch2_byte = GattAttribute::duration_schedule_to_bytes(sch2, 1);
  Mocks::set_watering_cycle(*fix.head, 0, sch1.cycle);
  Mocks::set_node_duration(*fix.head, 0, sch1.duration);
  Mocks::set_watering_cycle(*fix.head, 1, sch2.cycle);
  Mocks::set_node_duration(*fix.head, 1, sch2.duration);
  GattAttribute attr{*fix.head};
  // End Initialization

  SECTION("duration schedule can be converted to bytes and vice versa") {
    NodeTypes::DurationSchedule schedule{
        .duration = 500,
        .cycle = {true, true, true, false, false, false, true}};
    FlatSchedule dur_bytes =
        GattAttribute::duration_schedule_to_bytes(schedule, test_address);
    auto re_schedule = GattAttribute::bytes_to_duration_schedule(dur_bytes);

    REQUIRE(dur_bytes[0] == test_address);
    REQUIRE(schedule.cycle == re_schedule.cycle);
  }

  SECTION("duration buffers are loaded propertly") {

    auto status = attr.load_duration_buffer(0);
    REQUIRE(attr.duration_buffer == sch1_byte);

    auto status2 = attr.load_duration_buffer(1);
    REQUIRE(attr.duration_buffer == sch2_byte);
    REQUIRE(status == BLE::Status::OP_OK);

    SECTION("does not load invalid node") {
      auto res = attr.load_duration_buffer(6);
      REQUIRE(res == BLE::Status::INVALID_NODE);
      REQUIRE(attr.duration_buffer == sch2_byte);
    }
  }
  SECTION("Testing handle_incoming_write main entrypoint") {
    SECTION("handles load row operations") {
      auto load_row1 = BleMocks::pkt_load_row();
      load_row1[BLE::TGT_ROW_IDX] = 3;
      BLE::Status res1 = handle_and_process_req(*fix.head, attr, load_row1);
      check_buffer_duration(fix, attr, 3);

      REQUIRE(res1 == BLE::Status::OP_OK);
      load_row1[BLE::TGT_ROW_IDX] = 2;

      handle_and_process_req(*fix.head, attr, load_row1);
      check_buffer_duration(fix, attr, 2);
      SECTION("will not execute invalid rows") {
        load_row1[BLE::TGT_ROW_IDX] = config::max_nodes + 1;

        BLE::Status res3 = handle_and_process_req(*fix.head, attr, load_row1);
        REQUIRE(res3 == BLE::Status::INVALID_NODE);
      }
    }
    SECTION("Handles write cell commands") {
      auto cell1 = BleMocks::pkt_write_cell();
      uint16_t val = Util::get_le16(&cell1[BLE::DATA_START_IDX]);
      auto target_node = cell1[BLE::TGT_ROW_IDX];

      auto durations1 = fix.head->get_node_hose_duration(target_node);

      BLE::Status res = handle_and_process_req(*fix.head, attr, cell1);
      attr.load_duration_buffer(target_node);
      // TODO: find a way to be able to test when the request has been processed
      // to load the duration buffer
      auto durations2 = fix.head->get_node_hose_duration(target_node);
      std::cout << "durations 1:" << durations1.value() << "\n";

      std::cout << "durations 2:" << durations2.value() << "\n";
      REQUIRE(durations1.value() != durations2.value());
      REQUIRE(val == durations2.value());
      Logger::log_simple("target node is %d", target_node);

      check_buffer_duration(fix, attr, target_node);
    }
    SECTION("invalid commands not processed") {

      auto row_add = BleMocks::pkt_write_cell();
      row_add[0] = 9;

      BLE::Status res = handle_and_process_req(*fix.head, attr, row_add);
      REQUIRE(res == BLE::Status::INVALID_CMD);
    }
    SECTION("invalid packet lengths not processed") {

      auto cel_add = BleMocks::pkt_write_cell();
      auto load = BleMocks::pkt_load_row();
      std::array<std::span<uint8_t>, 2> vals = {cel_add, load};
      auto buff1 = attr.duration_buffer;
      bool failed = false;
      for (auto &pkt : vals) {
        pkt[BLE::DATA_LEN_IDX] += 1;

        BLE::Status rc = handle_and_process_req(*fix.head, attr, pkt);
        if (rc != BLE::Status::INVALID_PKT_LEN) {
          failed = true;
        }
      }

      auto buff2 = attr.duration_buffer;
      REQUIRE(failed == false);
      REQUIRE(buff1 == buff2);
    }
  }
}
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

void check_buffer_duration(HeadFixture &fix, GattAttribute &attr,
                           size_t node_index) {
  auto check1 = fix.head->get_node_duration_schedule(node_index);
  auto check2 = GattAttribute::bytes_to_duration_schedule(attr.duration_buffer);
  if (check1.value() != check2) {
    std::cout << "from head: " << check1.value() << "\n";
    std::cout << "from duration buff: " << check2 << "\n";
  }
  REQUIRE(check1.value() == check2);
}
