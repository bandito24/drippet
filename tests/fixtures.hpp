#pragma once

#include "mocks.hpp"
#include <memory>

#include "constants.hpp"
#include "self_node.hpp"

#include "clock.hpp"
#include "head.hpp"
#include "node.hpp"
#include "switch.hpp"
#include <chrono>
#include <fakeit.hpp>

using namespace fakeit;
struct MockFn {
  virtual void do_it() = 0;
};

struct HeadFixture {
  fakeit::Mock<MockFn> mockReset;
  fakeit::Mock<iClock> clockMock;
  fakeit::Mock<Storage> storageMock;

  std::unique_ptr<Head> head;

  HeadFixture(Time::Long phase_length = Time::Long{86400}) {

    When(Method(mockReset, do_it)).AlwaysReturn();
    When(Method(storageMock, save_durations)).AlwaysReturn();
    When(Method(storageMock, read_boot_durations))
        .AlwaysReturn(NodeTypes::DurationSchedule{});

    When(Method(clockMock, is_watering_due)).AlwaysReturn();
    When(Method(clockMock, progress_watering_due)).AlwaysReturn();
    When(Method(clockMock, now)).AlwaysReturn();
    When(Method(clockMock, get_phase_length)).AlwaysReturn(phase_length);
    head = std::make_unique<Head>(clockMock.get(), storageMock.get(),
                                  [this]() { this->mockReset.get().do_it(); });
  };
};
struct SelfNodeFixture {
  fakeit::Mock<SteadyClock> steady_clock;
  std::unique_ptr<SelfNode> self_node;
  std::unique_ptr<SolenoidMock> sol = std::make_unique<SolenoidMock>();
  std::unique_ptr<Switch> downstream = std::make_unique<SwitchMock>();

  void set_mock_now(Time::Long return_time) {
    // Custom
    When(Method(this->steady_clock, now)).AlwaysReturn(return_time);
  }
  SelfNodeFixture() {

    // Default
    set_mock_now(500);
    this->self_node = std::make_unique<SelfNode>(
        steady_clock.get(), std::move(sol), std::move(downstream));
  };
};
constexpr Time::Long TEST_PHASE_LEN = 100;

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
  void initialize() {

    setenv("TZ", "UTC", 1);
    tzset();

    When(Method(sysMock, change_system_clock)).AlwaysReturn();
    this->set_test_time(ClockFixture::CURR_TIME_HR,
                        ClockFixture::CURR_TIME_MIN);
    When(Method(sysMock, now)).AlwaysDo([this]() { return this->curr_time; });
  }

  ClockFixture() : espClock{TEST_PHASE_LEN, sysMock.get()} {
    this->initialize();
  }
  ClockFixture(Time::Long phase_length)
      : espClock{phase_length, sysMock.get()} {
    this->initialize();
  }
};
