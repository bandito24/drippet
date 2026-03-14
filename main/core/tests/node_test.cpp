#include "mocks.hpp"
#include "node.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

TEST_CASE("unit functions", "[node_unit]") {
  SECTION("will recognize if a node has no watering set") {

    Node node{Mocks::sample_key};
    bool all_zero = node.all_durations_zero();
    REQUIRE(all_zero == true);
  }
}
TEST_CASE("config functions", "[node_config]") {

  SECTION("can set the node durations") {

    Node node{Mocks::sample_key};
    NodeTypes::HoseDurations durations{1, 2, 3, 4, 5};
    node.set_node_durations(durations);
    REQUIRE(node.get_all_hose_durations() == durations);
  }
}
