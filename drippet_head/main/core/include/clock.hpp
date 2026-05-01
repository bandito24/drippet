#pragma once
#include "constants.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <sys/time.h>

//
struct iClock {
  virtual ~iClock() = default;
  virtual Time::Time_Point now() const = 0;

  virtual bool is_watering_due() const = 0;
  virtual void progress_watering_due() = 0;
  virtual Weekdays get_day_of_week() const = 0;
};
std::unique_ptr<iClock> initialize_clock();

struct iSysClock {

  virtual ~iSysClock() = default;
  virtual Time::Time_Point now() const = 0;
  virtual void change_system_clock(const timeval &time) = 0;
};
class SystemClock : public iSysClock {
  void change_system_clock(const timeval &time) override;
  Time::Time_Point now() const override;
};

class Esp32Clock : public iClock {
public:
  Time::Time_Point now() const override;
  time_t set_time(int year, int month, int day, int hour, int min,
                  int second = 0);
  time_t set_daily_watering(uint8_t hour, uint8_t min);
  Time::WateringSchedule watering_schedule{};
  Esp32Clock(iSysClock &sysClock) : sys{sysClock} {};
  static time_t makeTime(int year, int month, int day, int hour, int min,
                         int second);
  std::optional<Time::Time_Point> get_next_watering_point() {
    return this->next_watering_point;
  }

  Weekdays get_day_of_week() const override {
    // TODO: implement this
    return Weekdays::MONDAY;
  };
  bool is_watering_due() const override;
  void progress_watering_due() override;

private:
  std::optional<Time::Time_Point> next_watering_point = std::nullopt;
  iSysClock &sys;
  bool initalized = false;
};
