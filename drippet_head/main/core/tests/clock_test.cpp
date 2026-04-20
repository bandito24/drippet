

#include "clock.hpp"
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

    clock.set_daily_watering(Fix::CURR_TIME_HR + 1, Fix::CURR_TIME_MIN);
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

    clock.set_daily_watering(Fix::CURR_TIME_HR - 1, Fix::CURR_TIME_MIN);

    Time::Time_Point now_point = fix.espClock.now();
    auto water_point = fix.espClock.get_next_watering_point();
    REQUIRE((now_point + std::chrono::hours{23}) ==
            *water_point); // Moved to the next_day
  }
}
