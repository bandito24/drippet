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

    When(Method(storageMock, init)).AlwaysReturn();
    When(Method(storageMock, read_boot_durations))
        .AlwaysReturn(NodeTypes::DurationSchedule{});

    When(Method(clockMock, is_watering_due)).AlwaysReturn();
    When(Method(clockMock, recalculate_next_watering_point)).AlwaysReturn();
    When(Method(clockMock, now)).AlwaysReturn();

    When(Method(clockMock, set_next_phase_start_time)).AlwaysReturn();
    When(Method(clockMock, get_phase_of_cycle))
        .AlwaysReturn(CyclePhase::FIRST);

    When(Method(clockMock, get_hourmin_curr_time))
        .AlwaysReturn(HourMin{12, 12});

    When(Method(clockMock, get_hourmin_next_phase))
        .AlwaysReturn(HourMin{12, 12});
    When(Method(clockMock, set_time)).AlwaysReturn();
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
  // Test date is 2026-03-03 (a Tuesday), after Time::MIN_VALID_EPOCH
  static constexpr std::chrono::sys_days TEST_DAY =
      std::chrono::year{2026} / std::chrono::March / 3;
  Time::Time_Point curr_time;
  Time::Time_Point get_curr_time() { return this->curr_time; }

  // Syncs the clock so that LOCAL time is hour:min on TEST_DAY
  ActionStatus set_test_time(int hour, int min, int offset_min = 0) {
    Time::Time_Point utc = Clk::make_ex_tp(hour, min) -
                           std::chrono::minutes{offset_min};
    return this->espClock.set_time(std::chrono::system_clock::to_time_t(utc),
                                   offset_min);
  }

  using Clk = ClockFixture;
  static time_t make_ex_time(int hour = Clk::CURR_TIME_HR,
                             int min = Clk::CURR_TIME_MIN) {
    return std::chrono::system_clock::to_time_t(Clk::make_ex_tp(hour, min));
  }
  // hour:min on TEST_DAY, as UTC
  static Time::Time_Point make_ex_tp(int hour = Clk::CURR_TIME_HR,
                                     int min = Clk::CURR_TIME_MIN) {
    return Time::Time_Point{TEST_DAY + std::chrono::hours{hour} +
                            std::chrono::minutes{min}};
  }
  void initialize() {

    setenv("TZ", "UTC", 1);
    tzset();

    // The mocked system clock follows whatever set_time writes to it
    When(Method(sysMock, change_system_clock)).AlwaysDo([this](const timeval &tv) {
      this->curr_time = std::chrono::system_clock::from_time_t(tv.tv_sec);
    });
    When(Method(sysMock, now)).AlwaysDo([this]() { return this->curr_time; });
    this->set_test_time(ClockFixture::CURR_TIME_HR,
                        ClockFixture::CURR_TIME_MIN);
  }

  ClockFixture() : espClock{TEST_PHASE_LEN, sysMock.get()} {
    this->initialize();
  }
  ClockFixture(Time::Long phase_length)
      : espClock{phase_length, sysMock.get()} {
    this->initialize();
  }
};

struct HeadFixtureClockActive {

  fakeit::Mock<MockFn> mockReset;
  ClockFixture clockFixture{};
  fakeit::Mock<Storage> storageMock;

  std::unique_ptr<Head> head;

  HeadFixtureClockActive(Time::Long phase_length = Time::Long{86400}) {

    When(Method(mockReset, do_it)).AlwaysReturn();
    When(Method(storageMock, save_durations)).AlwaysReturn();
    When(Method(storageMock, read_boot_durations))
        .AlwaysReturn(NodeTypes::DurationSchedule{});

    head = std::make_unique<Head>(clockFixture.espClock, storageMock.get(),
                                  [this]() { this->mockReset.get().do_it(); });
  };
};
