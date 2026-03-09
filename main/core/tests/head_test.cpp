
#include "config.hpp"
#include "driver.hpp"
#include "fixtures.hpp"
#include "head.hpp"
#include "node.hpp"
#include "protocol.hpp"
#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <functional>
#include <optional>

using Fixture = HeadFixture;
constexpr NodeKey_t TEST_KEY = 500;

TEST_CASE("create node pending behaves correctly", "[head]") {

  SECTION("Node Links all start as nullptr") {

    Fixture fix;
    Head head = std::move(fix.head);

    bool all_null = true;
    for (std::size_t i = 0; i < config::max_nodes; i++) {
      if (head.get_node(i) != nullptr) {
        all_null = false;
        break;
      }
    }
    REQUIRE(all_null == true);
  }
  SECTION("successfully creates a node in initializing state") {

    Fixture fix;
    Head head = std::move(fix.head);
    auto address = head.create_node_pending(TEST_KEY);
    REQUIRE(*address == 0); // First address location;
    REQUIRE(head.get_node(0)->get_node_status() == NodeStatus::INITIALIZING);
  }
  SECTION("head does not allow more than max addresses") {

    Fixture fix;
    Head head = std::move(fix.head);
    bool add_correct = true;
    for (size_t i = 0; i < config::max_nodes; i++) {
      auto address = head.create_node_pending(i);
      if (!address) {
        add_correct = false;
        break;
      }
    }
    REQUIRE(add_correct);
    auto last_address = head.create_node_pending(TEST_KEY);

    auto last_address2 = head.create_node_pending(TEST_KEY + 5);
    REQUIRE(last_address == config::max_nodes);
    REQUIRE(last_address2 ==
            config::max_nodes); // Means it full--this return means full
  }
  SECTION("confirm node pending sets a node as ready if key and address match "
          "for valid index") {
    Fixture fix;
    Head head = std::move(fix.head);
    auto address = head.create_node_pending(TEST_KEY);
    auto addressAgain = head.create_node_pending(TEST_KEY);
    REQUIRE(addressAgain == std::nullopt);

    auto address2 = head.create_node_pending(TEST_KEY + 1);

    auto address3 = head.create_node_pending(TEST_KEY + 2);
    auto status1 = head.confirm_node_pending(TEST_KEY + 1, 0);
    REQUIRE(status1 == NodeLinkStatus::LINK_KEY_MISMATCH);
    REQUIRE(head.get_node(1)->get_node_status() == NodeStatus::INITIALIZING);

    auto status2 = head.confirm_node_pending(TEST_KEY + 1, 1);
    REQUIRE(status2 == NodeLinkStatus::LINK_OK);
    REQUIRE(head.get_node(1)->get_node_status() == NodeStatus::READY);
    REQUIRE(head.get_node(0)->get_node_status() == NodeStatus::INITIALIZING);
    REQUIRE(head.get_node(2)->get_node_status() == NodeStatus::INITIALIZING);
  }
}
