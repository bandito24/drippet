
#include "config.hpp"
#include "node.hpp"
#include "time.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

TEST_CASE("Node Test", "[node]") {
  SECTION("Initialzes array with proper watering_interval and hose_durations") {
    config::Address add = 5;
    config::HoseDurations durations =
        NodeTypes::create_durations({1, 2, 3, 4, 5});
    NodeTypes::Node_Link new_node =
        std::make_unique<Node>(Time::Day_In_Seconds, durations, add);
    REQUIRE(new_node->get_node_status() == NodeStatus::INITIALIZING);
    REQUIRE(new_node->get_address() == add);
    NodeTypes::HoseDurations arrCheck = new_node->get_all_hose_durations();
    REQUIRE(std::equal(durations.begin(), durations.end(), arrCheck.begin(),
                       arrCheck.end()));
    REQUIRE(new_node->get_watering_interval() == Time::Day_In_Seconds);
  };
  SECTION("Cannot Initialize Array with total_hose_duration greater than "
          "watering_interval") {

    auto fail_duration = Time::Time_Seconds{4};
    config::HoseDurations durations =
        NodeTypes::create_durations({1, 1, 1, 1, 1});
    NodeTypes::Node_Link new_node =
        std::make_unique<Node>(fail_duration, durations, config::Address{5});
    REQUIRE(new_node->get_node_status() == NodeStatus::ERR_DURATION);
  }
}
