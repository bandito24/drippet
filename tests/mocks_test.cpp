
#include "config.hpp"
#include "constants.hpp"
#include "fixtures.hpp"
#include "logger.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include "protocol_types.hpp"
#include "util.hpp"

#include <array>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <fakeit.hpp>
#include <optional>

TEST_CASE("mocks test", "[mocks]") {
  HeadFixture fix{};

  SECTION("populate head nodes creates desired count with unique time") {
    // NOTE: This tests populate single node function through extension
    Mocks::populate_head_nodes(*fix.head, 4);
    bool failed = false;
    std::optional<uint16_t> last_dur = std::nullopt;
    for (size_t i = 0; i < 4; i++) {
      auto curr_durr = fix.head->get_node_hose_duration(i);
      auto status = fix.head->get_node_status(i);
      if (!curr_durr || status != NodeStatus::READY || *curr_durr == 0) {
        failed = true;
      }
      if (last_dur && (*curr_durr == *last_dur)) {
        failed = true;
      }
      last_dur = curr_durr;
    }
    REQUIRE(failed == false);

    SECTION("set node status behaves as expected") {
      Mocks::set_node_status(*fix.head, 0, NodeStatus::ERR);
      REQUIRE(fix.head->get_node_status(0) == NodeStatus::ERR);
    }
  }
  SECTION("Populate head nodes creates initializing node") {
    auto msg = Mocks::create_node_pending(*fix.head, 500);
    REQUIRE(fix.head->get_node_status(0) == NodeStatus::INITIALIZING);
    REQUIRE(fix.head->get_node_addr_by_key(500) == 0);

    SECTION("Confirm Node Pending has a ready node") {
      Mocks::confirm_node_pending(*fix.head, msg->address, 500);

      REQUIRE(fix.head->get_node_status(0) == NodeStatus::READY);
      REQUIRE(fix.head->get_node_addr_by_key(500) == 0);
    }
  }
  SECTION("create and confirm head nodes works") {
    Mocks::create_and_confirm_node(*fix.head, 300);

    REQUIRE(fix.head->get_node_status(0) == NodeStatus::READY);
    REQUIRE(fix.head->get_node_addr_by_key(300) == 0);
  }
  SECTION("set watering cycle mock works") {

    Mocks::create_and_confirm_node(*fix.head, 300);
    NodeTypes::WateringCycle cycle{true, false, false, true, true, true, false};
    Mocks::set_watering_cycle(*fix.head, 0, cycle);
    auto check = fix.head->get_node_duration_schedule(0);
    REQUIRE(check->cycle == cycle);
  }
}
