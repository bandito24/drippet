#pragma once
#include "constants.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <sys/time.h>

//
constexpr int DEFAULT_YEAR = 2026;
constexpr int DEFAULT_MONTH = 12;
constexpr int DEFAULT_DAY = 12;

struct HourMin {
  uint8_t hour;
  uint8_t minute;
  bool operator==(const HourMin &other) const {
    return this->hour == other.hour && this->minute == other.minute;
  }
};
struct iSysClock {

  virtual ~iSysClock() = default;
  virtual Time::Time_Point now() const = 0;
  virtual void change_system_clock(const timeval &time) = 0;
};
class SystemClock : public iSysClock {
  void change_system_clock(const timeval &time) override;
  Time::Time_Point now() const override;
};

struct iClock {
  virtual ~iClock() = default;
  virtual Time::Time_Point now() const = 0;

  virtual bool is_watering_due() const = 0;
  virtual void progress_watering_due() = 0;

  CyclePhase phase_of_cycle = CyclePhase::FIRST;
  // TEST: This to make sure it doesn't conflict with watering
  void set_phase_of_cycle(CyclePhase phase) { this->phase_of_cycle = phase; };

  CyclePhase get_phase_of_cycle() const { return this->phase_of_cycle; };
  virtual Time::Long get_phase_length() const = 0;
  virtual time_t set_next_phase_start_time(uint8_t hour, uint8_t min) = 0;

  virtual time_t set_time(int hour, int min) = 0;
  virtual HourMin get_hourmin_curr_time() const = 0;
  virtual std::optional<HourMin> get_hourmin_next_phase() const = 0;

  ActionStatus set_int_phase_of_cycle(uint8_t enumIdx) {
    if (enumIdx >= static_cast<uint8_t>(CyclePhase::CYCLE_LEN)) {
      return ActionStatus::INVALID_TIME;
    }
    CyclePhase phase = static_cast<CyclePhase>(enumIdx);
    this->set_phase_of_cycle(phase);
    return ActionStatus::OK;
  }
};

class Esp32Clock : public iClock {
public:
  Esp32Clock(Time::Long _phase_length, iSysClock &_sys)
      : phase_length{_phase_length}, sys{_sys} {};
  Time::Time_Point now() const override;
  time_t set_time(int hour, int min) override;
  time_t set_next_phase_start_time(uint8_t hour, uint8_t min) override;
  static time_t makeTime(int year, int month, int day, int hour, int min,
                         int second);
  std::optional<Time::Time_Point> get_next_watering_point() {
    return this->next_watering_point;
  }
  Time::Long get_phase_length() const override { return this->phase_length; }
  bool is_watering_due() const override;
  void progress_watering_due() override;

  HourMin get_hourmin_curr_time() const override;
  std::optional<HourMin> get_hourmin_next_phase() const override;

private:
  std::optional<Time::Time_Point> next_watering_point = std::nullopt;
  Time::Long phase_length;
  HourMin cast_hour_min(const Time::Time_Point &point) const;
  iSysClock &sys;
  bool initalized = false;
};
