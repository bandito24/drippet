#include "clock.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fakeit.hpp>
#include <functional>

TEST_CASE("unit functions", "[node_unit]") {
  SECTION("will recognize if a node has no watering set") {

    Node node{Mocks::sample_key};
    bool all_zero = node.all_durations_zero();
    REQUIRE(all_zero == true);
  }
}
TEST_CASE("config functions", "[node_config]") {

  using namespace std::chrono;
  constexpr NodeTypes::HoseDurations empty_dur{0, 0, 0, 0, 0};

  SECTION("can set the node durations") {

    Node node{Mocks::sample_key};
    auto dur = config::MAX_HOSE_DURATION;
    NodeTypes::HoseDurations durations{5, 4, 3, 2, dur};
    ActionStatus status = node.set_node_durations(durations);
    REQUIRE(node.get_all_hose_durations() == durations);
    REQUIRE(status == ActionStatus::OK);

    SECTION("get_hose_duration returns correct index") {

      for (size_t addr = 0; addr < config::node_hose_count; addr++) {
        REQUIRE(durations[addr] == node.get_hose_duration(addr));
      }
    }
    SECTION("can not edit a duration with too long of a time") {
      auto durations2 = durations;
      ActionStatus status = node.edit_hose_duration(1, dur + 1);
      REQUIRE(status == ActionStatus::INVALID_TIME);
      REQUIRE(node.get_all_hose_durations() == durations);
    }
    SECTION("can edit a durations with valid times") {

      auto durations2 = durations;
      durations2[0] = dur - 5;
      ActionStatus status = node.edit_hose_duration(0, durations2[0]);
      REQUIRE(status == ActionStatus::OK);
      REQUIRE(node.get_all_hose_durations() == durations2);
    }
  }
  SECTION("can not set node durations longer than max permitted") {

    Node node{Mocks::sample_key};
    Time::Time_Seconds duration = config::MAX_HOSE_DURATION + 1;
    NodeTypes::HoseDurations durations{duration, 0, 0, 0, 0};
    ActionStatus status = node.set_node_durations(durations);
    REQUIRE(status == ActionStatus::INVALID_TIME);
    REQUIRE(node.get_all_hose_durations() == empty_dur);
  }
}
