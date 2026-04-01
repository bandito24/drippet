#include "config.hpp"
#include "fixtures.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "mocks.hpp"
#include "util.hpp"

#include <array>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

void check_buffer_duration(HeadFixture &fix, GattAttribute &attr,
                           size_t node_index);

TEST_CASE("BLE tests", "[ble]") {
  HeadFixture fix;
  Mocks::populate_head_nodes(fix.head, 5);
  GattAttribute attr{fix.head};

  SECTION("duration buffers are loaded propertly") {
    auto res = attr.load_duration_buffer(0);

    REQUIRE(res == BLE::Status::OP_OK);
    check_buffer_duration(fix, attr, 0);

    auto res2 = attr.load_duration_buffer(1);
    check_buffer_duration(fix, attr, 1);
    REQUIRE(res2 == BLE::Status::OP_OK);

    SECTION("does not load invalid node") {
      auto res = attr.load_duration_buffer(6);
      REQUIRE(res == BLE::Status::INVALID_NODE);
      check_buffer_duration(fix, attr, 1);
    }
  }
  SECTION("set_node_duration alters head node durations") {
    auto node = fix.head.get_node(2);
    auto dur1 = node->get_all_hose_durations();
    attr.set_node_duration(2, 1, 900);
    auto dur2 = node->get_all_hose_durations();
    REQUIRE(dur1 != dur2);
    REQUIRE(dur2[1] == 900);
  }
  SECTION("Testing handle_incoming_write main entrypoint") {
    SECTION("handles load row operations") {
      auto load_row1 = BleMocks::pkt_load_row;
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
  }
}

void check_buffer_duration(HeadFixture &fix, GattAttribute &attr,
                           size_t node_index) {

  std::array<uint8_t, config::node_hose_count * 2> check{};
  auto durations = fix.head.get_node(node_index)->get_all_hose_durations();
  Util::le16_to_le8(check, durations);

  std::array<uint8_t, config::node_hose_count * 2> buff{};
  std::copy(attr.duration_buffer.begin() + 1, attr.duration_buffer.end(),
            buff.begin());
  REQUIRE(check == buff);
  REQUIRE(attr.duration_buffer[0] == node_index);
}
