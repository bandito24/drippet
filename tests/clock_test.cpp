

#include "clock.hpp"
#include "constants.hpp"
#include "fixtures.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fakeit.hpp>
#include <optional>

using Fix = ClockFixture;
using namespace std::chrono;
constexpr int ONE_HR = 60 * 60;
constexpr Time::Long day_seconds = duration_cast<seconds>(days{1}).count();

TEST_CASE("Clock and next watering durations set correctly", "[clock]") {

  SECTION("adjusts the next_watering_ts day ahead if time is moved after "
          "curr_time") {

    Fix fix{day_seconds};
    Esp32Clock &clock = fix.espClock;
    auto &sysMock = fix.sysMock;
    REQUIRE(clock.get_next_watering_point() == std::nullopt);

    clock.set_next_phase_start_time(Fix::CURR_TIME_HR + 1, Fix::CURR_TIME_MIN);
    SECTION("Make sure hourmin helper function works for seeing phase") {
      REQUIRE(
          (clock.get_hourmin_next_phase()).value() ==
          HourMin{.hour = Fix::CURR_TIME_HR + 1, .minute = Fix::CURR_TIME_MIN});
    }
    auto first_watering = fix.espClock.get_next_watering_point();

    REQUIRE((fix.espClock.now() + std::chrono::hours{1}) == *first_watering);

    // Tests to make sure if the user changes the time to be later than the next
    // watering it doesn't just start watering immediately and sitches to the
    // next day
    fix.set_test_time(Fix::CURR_TIME_HR + 2, Fix::CURR_TIME_MIN);

    SECTION("Make sure hourmin helper function works for seeing time") {
      REQUIRE(
          clock.get_hourmin_curr_time() ==
          HourMin{.hour = Fix::CURR_TIME_HR + 2, .minute = Fix::CURR_TIME_MIN});
    }

    auto second_watering = fix.espClock.get_next_watering_point();
    REQUIRE((*first_watering + std::chrono::days{1}) == *second_watering);
  }
  SECTION("Adjusts next_watering_ts is next_watering_ts is moved before "
          "curr_time") {

    Fix fix{day_seconds};
    Esp32Clock &clock = fix.espClock;
    auto &sysMock = fix.sysMock;

    clock.set_next_phase_start_time(Fix::CURR_TIME_HR - 1, Fix::CURR_TIME_MIN);

    Time::Time_Point now_point = fix.espClock.now();
    auto water_point = fix.espClock.get_next_watering_point();

    REQUIRE((now_point + std::chrono::hours{23}) ==
            *water_point); // Moved to the next_day
  }
  SECTION("next watering point advances when watering is due") {

    Fix fix{};
    Esp32Clock &clock = fix.espClock;
    REQUIRE(clock.get_next_watering_point() == std::nullopt);

    clock.set_next_phase_start_time(Fix::CURR_TIME_HR, Fix::CURR_TIME_MIN + 1);
    REQUIRE(clock.is_watering_due() == false);
    fix.curr_time += minutes{1};
    REQUIRE(clock.is_watering_due() == true);

    SECTION("recalculating does nothing if watering is not due") {
      fix.curr_time -= minutes{1};
      auto prev = clock.get_next_watering_point();
      clock.recalculate_next_watering_point();
      REQUIRE(prev == clock.get_next_watering_point());
    }
    SECTION("recalculating after watering moves to the next phase") {
      auto water_point = *clock.get_next_watering_point();
      for (int i = 1; i <= 3; i++) {
        clock.recalculate_next_watering_point();
        REQUIRE(*clock.get_next_watering_point() ==
                water_point + seconds{clock.get_phase_length() * i});
        REQUIRE(clock.is_watering_due() == false);
        fix.curr_time += seconds{clock.get_phase_length()};
        REQUIRE(clock.is_watering_due() == true);
      }
    }
  }
}

TEST_CASE("Clock uses UTC with a local offset", "[clock]") {

  constexpr int PDT = -420;
  constexpr int PST = -480;

  SECTION("phase start is local time and stored as UTC") {
    Fix fix{day_seconds};
    Esp32Clock &clock = fix.espClock;
    // 14:30 local = 21:30 UTC on the test day
    REQUIRE(fix.set_test_time(14, 30, PDT) == ActionStatus::OK);
    REQUIRE(fix.curr_time == Fix::make_ex_tp(21, 30));

    clock.set_next_phase_start_time(2, 0);
    // 02:00 local already passed today, so tomorrow 02:00 local = 09:00 UTC
    REQUIRE(*clock.get_next_watering_point() ==
            Fix::make_ex_tp(9, 0) + days{1});

    SECTION("hourmin helpers report local time") {
      REQUIRE(clock.get_hourmin_curr_time() == HourMin{14, 30});
      REQUIRE(clock.get_hourmin_next_phase() == HourMin{2, 0});
    }
    SECTION("a DST offset change keeps the local watering time") {
      time_t utc_now = system_clock::to_time_t(fix.curr_time);
      REQUIRE(clock.set_time(utc_now, PST) == ActionStatus::OK);
      REQUIRE(*clock.get_next_watering_point() ==
              Fix::make_ex_tp(10, 0) + days{1});
      REQUIRE(clock.get_hourmin_next_phase() == HourMin{2, 0});
    }
  }

  SECTION("phase of cycle is the local weekday") {
    Fix fix{day_seconds};
    Esp32Clock &clock = fix.espClock;
    // 2026-03-03 is a Tuesday
    fix.set_test_time(12, 0);
    REQUIRE(clock.get_phase_of_cycle() == CyclePhase::THIRD);

    // 20:00 Tuesday local is already 03:00 Wednesday UTC
    fix.set_test_time(20, 0, PDT);
    REQUIRE(fix.curr_time == Fix::make_ex_tp(3, 0) + days{1});
    REQUIRE(clock.get_phase_of_cycle() == CyclePhase::THIRD);
  }

  SECTION("moving the clock backwards picks the next upcoming watering") {
    Fix fix{day_seconds};
    Esp32Clock &clock = fix.espClock;
    clock.set_next_phase_start_time(Fix::CURR_TIME_HR + 1, Fix::CURR_TIME_MIN);
    auto first_watering = *clock.get_next_watering_point();

    time_t two_days_back =
        system_clock::to_time_t(fix.curr_time - days{2});
    REQUIRE(clock.set_time(two_days_back, 0) == ActionStatus::OK);
    REQUIRE(*clock.get_next_watering_point() == first_watering - days{2});
  }

  SECTION("syncing exactly at the watering time schedules the next phase") {
    Fix fix{day_seconds};
    Esp32Clock &clock = fix.espClock;
    clock.set_next_phase_start_time(Fix::CURR_TIME_HR, Fix::CURR_TIME_MIN);
    REQUIRE(*clock.get_next_watering_point() == fix.curr_time + days{1});
  }

  SECTION("invalid time or offset is rejected and the clock is untouched") {
    Fix fix{day_seconds};
    Esp32Clock &clock = fix.espClock;
    clock.set_next_phase_start_time(Fix::CURR_TIME_HR + 1, Fix::CURR_TIME_MIN);
    auto before_time = fix.curr_time;
    auto before_watering = clock.get_next_watering_point();
    time_t valid_ts = system_clock::to_time_t(fix.curr_time);

    REQUIRE(clock.set_time(Time::MIN_VALID_EPOCH - 1, 0) ==
            ActionStatus::INVALID_TIME);
    REQUIRE(clock.set_time(valid_ts, Time::OFFSET_RANGES[0] - 1) ==
            ActionStatus::INVALID_TIME);
    REQUIRE(clock.set_time(valid_ts, Time::OFFSET_RANGES[1] + 1) ==
            ActionStatus::INVALID_TIME);
    REQUIRE(fix.curr_time == before_time);
    REQUIRE(clock.UTC_OFFSET_MIN == 0);
    REQUIRE(clock.get_next_watering_point() == before_watering);
  }

  SECTION("watering is never due before the first sync") {
    Fix fix{day_seconds};
    Esp32Clock unsynced{day_seconds, fix.sysMock.get()};
    unsynced.set_next_phase_start_time(Fix::CURR_TIME_HR, Fix::CURR_TIME_MIN + 1);
    fix.curr_time += days{2};
    REQUIRE(unsynced.is_watering_due() == false);
  }
}
