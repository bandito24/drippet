#include "config.hpp"
#include "fixtures.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"
#include "mocks.hpp"
#include "node.hpp"
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

TEST_CASE("BLE tests", "[ble]") {
  HeadFixture fix;
  Mocks::populate_head_nodes(*fix.head, 5);
  GattAttribute attr{*fix.head};

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
      uint16_t val = Util::get_le16(&cell1[BLE::TGT_CELL_IDX + 1]);
      auto target_node = cell1[BLE::TGT_ROW_IDX];
      auto target_hose = cell1[BLE::TGT_CELL_IDX];

      auto durations1 = fix.head->get_node_hose_durations(target_node);
      auto res = attr.handle_incoming_write(cell1);
      auto durations2 = fix.head->get_node_hose_durations(target_node);
      REQUIRE(durations1.value() != durations2.value());
      durations1.value()[target_hose] = val;
      REQUIRE(durations1.value() == durations2.value());
      check_buffer_duration(fix, attr, target_node);
    }
    SECTION("Handles write row commands") {
      auto row_add = BleMocks::pkt_write_row();
      NodeTypes::HoseDurations durations_add{};
      auto target_node = row_add[BLE::TGT_ROW_IDX];
      auto durations1 = fix.head->get_node_hose_durations(target_node);
      size_t addr = 0;
      for (size_t i = BLE::TGT_ROW_IDX + 1; i < row_add.size(); i += 2) {
        uint16_t val = Util::get_le16(&row_add[i]);
        durations_add[addr] = val;
        addr += 1;
      }
      auto rc1 = attr.handle_incoming_write(row_add);
      REQUIRE(rc1 == BLE::Status::OP_OK);
      auto durations2 = fix.head->get_node_hose_durations(target_node);
      REQUIRE(durations1.value() != durations2.value());
      REQUIRE(durations2.value() == durations_add);
      check_buffer_duration(fix, attr, target_node);
    }
    SECTION("invalid commands not processed") {

      auto row_add = BleMocks::pkt_write_row();
      row_add[0] = 9;
      auto res = attr.handle_incoming_write(row_add);
      REQUIRE(res == BLE::Status::INVALID_CMD);
    }
    SECTION("invalid packet lengths not processed") {

      auto row_add = BleMocks::pkt_write_row();
      auto cel_add = BleMocks::pkt_write_cell();
      auto load = BleMocks::pkt_load_row();
      std::array<std::span<uint8_t>, 3> vals = {row_add, cel_add, load};
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

  std::array<uint8_t, config::node_hose_count * 2> check{};

  auto durations = fix.head->get_node_hose_durations(node_index);

  Util::le16_to_le8(check, durations.value());

  std::array<uint8_t, config::node_hose_count * 2> buff{};
  std::copy(attr.duration_buffer.begin() + 1, attr.duration_buffer.end(),
            buff.begin());
  REQUIRE(check == buff);
  REQUIRE(attr.duration_buffer[0] == node_index);
}
