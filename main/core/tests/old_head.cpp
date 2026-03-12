
#include "config.hpp"
#include "fixtures.hpp"
#include "head.hpp"
#include "node.hpp"
#include "time.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <memory>
using namespace fakeit;
using Fixture = HeadFixture;
constexpr config::Address address_check = config::Address{255};

NodeLinkStatus populate_head(Head &head, config::Address address_add = 0) {
  config::Address address = address_check + address_add;
  return head.add_node(std::make_unique<Node>(
      Time::Day_In_Seconds, NodeTypes::create_durations({1, 1, 1, 1, 1, 1}),
      address));
}

TEST_CASE("Head Node", "[head]") {

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
  SECTION("Nodes are able to be added and removed") {

    Fixture fix;
    Head head = std::move(fix.head);
    NodeLinkStatus status = populate_head(head, 0);
    REQUIRE(status == NodeLinkStatus::LINK_OK);
    iNode *node_check = head.get_node(0);

    REQUIRE(node_check->get_address() == address_check);
    REQUIRE(node_check->get_node_status() == NodeStatus::INITIALIZING);
    REQUIRE(head.get_node_count() == 1);

    head.remove_node(0);

    iNode *node_check2 = head.get_node(0);

    REQUIRE(head.get_node_count() == 0);
    REQUIRE(node_check2 == nullptr);
  }
  SECTION("Cannot add more nodes than permitted max") {
    Fixture fix;
    Head head = std::move(fix.head);
    NodeLinkStatus last;
    for (std::size_t i = 0; i < config::max_nodes; i++) {
      last = populate_head(head, i);
    }
    REQUIRE(head.get_node_count() == config::max_nodes);
    REQUIRE(last == NodeLinkStatus::LINK_OK);
    NodeLinkStatus newest = populate_head(head, 10);
    REQUIRE(newest == LINK_FULL);
    REQUIRE(head.get_node_count() == config::max_nodes);
  }
  SECTION("Cannot add two nodes with the same physical address") {

    Fixture fix;
    Head head = std::move(fix.head);
    populate_head(head, 0);

    NodeTypes::Node new_node = std::make_unique<Node>(
        Time::Day_In_Seconds, NodeTypes::create_durations({1, 1, 1, 1, 1, 1}),
        address_check);
    NodeLinkStatus fail = head.add_node(std::move(new_node));
    REQUIRE(fail == NodeLinkStatus::LINK_ERR_ADDRESS);
    REQUIRE(head.get_node_count() == 1);
  }
}
