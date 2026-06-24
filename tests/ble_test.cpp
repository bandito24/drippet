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

TEST_CASE("BLE tests", "[ble]") {
  SKIP();
  HeadFixture fix;

  Mocks::populate_head_nodes(*fix.head, 5);
  NodeTypes::DurationSchedule sch1 = {.duration = 900,
                                      .cycle{true, false, true}};
  NodeTypes::DurationSchedule sch2 = {.duration = 1000,
                                      .cycle{false, true, false}};
  auto sch1_byte = GattAttribute::duration_schedule_to_bytes(sch1, 0);
  auto sch2_byte = GattAttribute::duration_schedule_to_bytes(sch2, 1);
  fix.head->set_watering_cycle(0, sch1.cycle);
  Mocks::set_node_duration(*fix.head, 0, sch1.duration);
  fix.head->set_watering_cycle(1, sch2.cycle);
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
      BLE::Status res1 = attr.handle_incoming_write(load_row1);
      check_buffer_duration(fix, attr, 3);

      REQUIRE(res1 == BLE::Status::OP_OK);
      load_row1[BLE::TGT_ROW_IDX] = 2;
      attr.handle_incoming_write(load_row1);
      check_buffer_duration(fix, attr, 2);
      SECTION("will not execute invalid rows") {
        load_row1[BLE::TGT_ROW_IDX] = config::max_nodes + 1;
        BLE::Status res3 = attr.handle_incoming_write(load_row1);
        REQUIRE(res3 == BLE::Status::INVALID_NODE);
      }
    }
    SECTION("Handles write cell commands") {
      auto cell1 = BleMocks::pkt_write_cell();
      uint16_t val = Util::get_le16(&cell1[BLE::DATA_START_IDX]);
      auto target_node = cell1[BLE::TGT_ROW_IDX];

      auto durations1 = fix.head->get_node_hose_duration(target_node);

      auto res = attr.handle_incoming_write(cell1);
      auto durations2 = fix.head->get_node_hose_duration(target_node);
      REQUIRE(durations1.value() != durations2.value());
      REQUIRE(val == durations2.value());
      check_buffer_duration(fix, attr, target_node);
    }
    SECTION("invalid commands not processed") {

      auto row_add = BleMocks::pkt_write_cell();
      row_add[0] = 9;
      auto res = attr.handle_incoming_write(row_add);
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
        auto rc = attr.handle_incoming_write(pkt);
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

void check_buffer_duration(HeadFixture &fix, GattAttribute &attr,
                           size_t node_index) {
  auto check1 = fix.head->get_node_duration_schedule(node_index);
  auto check2 = GattAttribute::bytes_to_duration_schedule(attr.duration_buffer);
  REQUIRE(check1.value() == check2);
}
