#pragma once

#include "clock.hpp"
#include "head.hpp"
#include <chrono>
#include <fakeit.hpp>

using namespace fakeit;

struct HeadFixture {
  fakeit::Mock<iValve> valveMock;
  fakeit::Mock<iClock> clockMock;
  Head head;

  HeadFixture() : head{valveMock.get(), clockMock.get()} {};
};
struct ClockFixture {
  fakeit::Mock<iSysClock> sysMock;
  Esp32Clock espClock;
  static constexpr int CURR_TIME_HR = 3;
  static constexpr int CURR_TIME_MIN = 0;
  Time::Time_Point curr_time;
  Time::Time_Point get_curr_time() { return this->curr_time; }
  void set_test_time(int hour, int min) {

    time_t time = Clk::make_ex_time(hour, min);
    this->curr_time = std::chrono::system_clock::from_time_t(time);
    time_t time2 = this->espClock.set_time(2026, 3, 3, hour, min);
  }

  using Clk = ClockFixture;
  static time_t make_ex_time(int hour = Clk::CURR_TIME_HR,
                             int min = Clk::CURR_TIME_MIN) {
    return Esp32Clock::makeTime(2026, 3, 3, hour, min, 0);
  }
  static Time::Time_Point make_ex_tp(int hour = Clk::CURR_TIME_HR,
                                     int min = Clk::CURR_TIME_MIN) {
    time_t time = Clk::make_ex_time(hour, min);
    return std::chrono::system_clock::from_time_t(time);
  }

  ClockFixture() : espClock{sysMock.get()} {

    setenv("TZ", "UTC", 1);
    tzset();

    When(Method(sysMock, change_system_clock)).AlwaysReturn();
    this->set_test_time(ClockFixture::CURR_TIME_HR,
                        ClockFixture::CURR_TIME_MIN);
    When(Method(sysMock, now)).AlwaysDo([this]() { return this->curr_time; });
  }
};
