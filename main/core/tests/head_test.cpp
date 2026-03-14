
#include "config.hpp"
#include "driver.hpp"
#include "fixtures.hpp"
#include "head.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
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

TEST_CASE("Head task behaves as expected in the task loop", "[head_task]") {

  SECTION("handle_incoming_frame updates state and emits the correct response "
          "from input") {
    Fixture fix;
    SECTION("reacts correctly to discovery incoming") {
      auto incoming = Mocks::incoming_discovery_frame(Mocks::sample_key);
      auto res = fix.head.handle_incoming_frame(incoming);
      REQUIRE(res != std::nullopt);
      auto r = *res;
      iNode *node = fix.head.get_node(0);
      REQUIRE(node->get_node_status() == NodeStatus::INITIALIZING);
      REQUIRE(r.address == 0);
      REQUIRE(r.data[0] == Mocks::sample_key);

      // Disregard duplicates
      auto res2 = fix.head.handle_incoming_frame(incoming);
      REQUIRE(res2 == std::nullopt);
      REQUIRE(fix.head.get_node(1) == nullptr);
      SECTION("reacts correctly to addressing incoming") {

        auto incoming2 = Mocks::incoming_adressing_frame(0, Mocks::sample_key);
        auto res3 = fix.head.handle_incoming_frame(incoming2);
        REQUIRE(node->get_node_status() == NodeStatus::READY);
        REQUIRE(res3 == std::nullopt);
      }
    }
  }
  SECTION("correctly processes watering schedule") {
    Fixture fix;
    When(Method(fix.clockMock, is_watering_due)).AlwaysReturn(true);
    When(Method(fix.clockMock, progress_watering_due)).AlwaysReturn();
    Mocks::populate_head_nodes(fix.head);
    config::Address add_check = config::max_nodes - 1;
    config::Address add_check_2 = config::max_nodes - 2;
    fix.head.get_node(add_check)->set_node_status(NodeStatus::INITIALIZING);
    fix.head.get_node(add_check_2)->set_node_durations({});
    fix.head.process_watering_schedule();

    SECTION("initialize_watering_states test") {
      // Fpr Making sure it doesnt queue empty or non-ready nodes

      for (size_t addr = 0; addr < config::max_nodes; addr++) {
        iNode *node = fix.head.get_node(addr);
        if (addr != add_check && addr != add_check_2) {
          REQUIRE(node->get_node_status() == NodeStatus::IN_QUEUE);
        } else if (addr == add_check) {
          REQUIRE(node->get_node_status() == NodeStatus::INITIALIZING);
        } else {
          REQUIRE(node->get_node_status() == NodeStatus::READY);
        }
      }
      REQUIRE(fix.head.get_head_status() == HeadStatus::WATERING_CMDS);

      Verify(Method(fix.clockMock, progress_watering_due)).Exactly(1);
    }
    SECTION("handling head state watering_cmds") {}
  }
}
