#include "clock.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fakeit.hpp>
#include <functional>

TEST_CASE("config functions", "[node_config]") {

  using namespace std::chrono;

  SECTION("can set the node durations") {

    Node node{Mocks::sample_key};
    auto dur = config::MAX_HOSE_DURATION;
    NodeTypes::HoseDuration test_dur = 5;
    ActionStatus status = node.set_node_duration(test_dur);
    REQUIRE(node.get_hose_duration() == test_dur);
    REQUIRE(status == ActionStatus::OK);
  }
  SECTION("clear retry count clears the recount") {

    Node node{Mocks::sample_key};
    node.increase_retry_count();
    uint8_t count = node.increase_retry_count();
    REQUIRE(count == 2);
    node.clear_retry_count();

    uint8_t count2 = node.increase_retry_count();
    REQUIRE(count2 == 1);
  }
}
