

#include "clock.hpp"
#include "constants.hpp"
#include "fixtures.hpp"
#include <__chrono/duration.h>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fakeit.hpp>
#include <optional>

using Fix = ClockFixture;
using namespace std::chrono;
constexpr int ONE_HR = 60 * 60;

TEST_CASE("Clock and next watering durations set correctly", "[clock]") {

  SECTION("adjusts the next_watering_ts day ahead if time is moved after "
          "curr_time") {

    Fix fix{};
    Esp32Clock &clock = fix.espClock;
    auto &sysMock = fix.sysMock;
    REQUIRE(clock.get_next_watering_point() == std::nullopt);

    clock.set_next_phase_start_time(Fix::CURR_TIME_HR + 1, Fix::CURR_TIME_MIN);
    auto first_watering = fix.espClock.get_next_watering_point();

    REQUIRE((fix.espClock.now() + std::chrono::hours{1}) == *first_watering);

    // Tests to make sure if the user changes the time to be later than the next
    // watering it doesn't just start watering immediately and sitches to the
    // next day
    fix.set_test_time(Fix::CURR_TIME_HR + 2, Fix::CURR_TIME_MIN);
    auto second_watering = fix.espClock.get_next_watering_point();
    REQUIRE((*first_watering + std::chrono::days{1}) == *second_watering);
  }
  SECTION("Adjusts next_watering_ts is next_watering_ts is moved before "
          "curr_time") {

    Fix fix{};
    Esp32Clock &clock = fix.espClock;
    auto &sysMock = fix.sysMock;

    clock.set_next_phase_start_time(Fix::CURR_TIME_HR - 1, Fix::CURR_TIME_MIN);

    Time::Time_Point now_point = fix.espClock.now();
    auto water_point = fix.espClock.get_next_watering_point();
    REQUIRE((now_point + std::chrono::hours{23}) ==
            *water_point); // Moved to the next_day
  }
  SECTION("correctly advances cycle phase and next watering point when "
          "watering is due") {

    Fix fix{};
    Esp32Clock &clock = fix.espClock;
    auto &sysMock = fix.sysMock;
    REQUIRE(clock.get_next_watering_point() == std::nullopt);

    SECTION("it will declare water is due")
    fix.set_test_time(Fix::CURR_TIME_HR, Fix::CURR_TIME_MIN);
    clock.set_next_phase_start_time(Fix::CURR_TIME_HR, Fix::CURR_TIME_MIN + 1);
    REQUIRE(clock.is_watering_due() == false);
    fix.curr_time += std::chrono::minutes{1};
    REQUIRE(clock.is_watering_due() == true);
    SECTION("progress watering due will do nothing if watering is not due") {
      fix.curr_time -= std::chrono::minutes{1};
      auto prev = clock.get_next_watering_point();
      clock.progress_watering_due();
      REQUIRE(prev == clock.get_next_watering_point());
    }
    SECTION("progress watering due will advance phase and watering point") {
      auto water_point = *clock.get_next_watering_point();
      int phase = static_cast<int>(clock.get_phase_of_cycle());

      // REQUIRE(static_cast<int>(clock.get_phase_of_cycle()) == 0);
      for (size_t i = 0; i < static_cast<size_t>(CyclePhase::CYCLE_LEN) - 1;
           i++) {
        clock.progress_watering_due();
        REQUIRE(static_cast<int>(clock.get_phase_of_cycle()) == ++phase);
        fix.curr_time += std::chrono::seconds{clock.get_phase_length()};
        REQUIRE(clock.is_watering_due() == true);
      }
      SECTION("test the cycle loops back around") {
        clock.progress_watering_due();
        REQUIRE(static_cast<int>(clock.get_phase_of_cycle()) == 0);
        fix.curr_time += std::chrono::seconds{clock.get_phase_length()};
        REQUIRE(clock.is_watering_due() == true);
      }

      // REQUIRE(clock.get_next_watering_point() ==
      //         clock.now() + std::chrono::seconds{clock.get_phase_length()});
    }
  }
}
