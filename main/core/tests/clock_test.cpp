

#include "clock.hpp"
#include "fixtures.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fakeit.hpp>

using Fix = ClockFixture;
using namespace std::chrono;
constexpr int ONE_HR = 60 * 60;

TEST_CASE("Clock and next watering durations set correctly", "[clock]") {

  SECTION("adjusts the next_watering_ts day ahead if time is moved after "
          "curr_time") {

    Fix fix{};
    Esp32Clock &clock = fix.espClock;
    auto &sysMock = fix.sysMock;
    REQUIRE(clock.get_next_watering_ts() == 0);

    clock.set_daily_watering(Fix::CURR_TIME_HR + 1, Fix::CURR_TIME_MIN);
    time_t first_ts = fix.espClock.get_next_watering_ts();
    time_t now_t = system_clock::to_time_t(fix.espClock.now());

    REQUIRE((now_t + (ONE_HR)) == first_ts);

    // Tests to make sure if the user changes the time to be later than the next
    // watering it doesn't just start watering immediately and sitches to the
    // next day
    fix.set_test_time(Fix::CURR_TIME_HR + 2, Fix::CURR_TIME_MIN);
    time_t second_ts = fix.espClock.get_next_watering_ts();
    REQUIRE((first_ts + (ONE_HR * 24)) == second_ts);
  }
  SECTION("Adjusts next_watering_ts is next_watering_ts is moved before "
          "curr_time") {

    Fix fix{};
    Esp32Clock &clock = fix.espClock;
    auto &sysMock = fix.sysMock;

    clock.set_daily_watering(Fix::CURR_TIME_HR - 1, Fix::CURR_TIME_MIN);

    time_t now_t = system_clock::to_time_t(fix.espClock.now());
    time_t water_ts = fix.espClock.get_next_watering_ts();
    REQUIRE((now_t + (ONE_HR * 23)) == water_ts); // Moved to the next_day
  }
}
