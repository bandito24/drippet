#pragma once
#include "constants.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <sys/time.h>

//

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
  CyclePhase get_phase_of_cycle() const { return this->phase_of_cycle; };
  virtual Time::Long get_phase_length() const = 0;
  virtual time_t set_next_phase_start_time(uint8_t hour, uint8_t min) = 0;

  virtual time_t set_time(int year, int month, int day, int hour, int min,
                          int second = 0) = 0;
};

class Esp32Clock : public iClock {
public:
  Esp32Clock(Time::Long _phase_length, iSysClock &_sys)
      : phase_length{_phase_length}, sys{_sys} {};
  Time::Time_Point now() const override;
  time_t set_time(int year, int month, int day, int hour, int min,
                  int second = 0) override;
  time_t set_next_phase_start_time(uint8_t hour, uint8_t min) override;
  static time_t makeTime(int year, int month, int day, int hour, int min,
                         int second);
  std::optional<Time::Time_Point> get_next_watering_point() {
    return this->next_watering_point;
  }
  Time::Long get_phase_length() const override { return this->phase_length; }
  bool is_watering_due() const override;
  void progress_watering_due() override;

private:
  std::optional<Time::Time_Point> next_watering_point = std::nullopt;
  Time::Long phase_length;
  iSysClock &sys;
  bool initalized = false;
};
