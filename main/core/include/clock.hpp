#pragma once
#include <chrono>
#include <memory>
#include <optional>
#include <sys/time.h>

namespace Time {
using Time_Point = std::chrono::system_clock::time_point;
using Time_Seconds = uint16_t;
constexpr Time_Seconds No_Time = Time_Seconds{0};

constexpr Time_Seconds Day_In_Seconds = static_cast<Time_Seconds>(24 * 60 * 60);
struct WateringSchedule {
  uint8_t hour;
  uint8_t minute;
};

} // namespace Time
  //
struct iClock {
  virtual ~iClock() = default;
  virtual Time::Time_Point now() const = 0;

  virtual bool is_watering_due() const;
  virtual void progress_watering_due();
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
  bool is_watering_due() const override;
  void progress_watering_due() override;

private:
  std::optional<Time::Time_Point> next_watering_point = std::nullopt;
  iSysClock &sys;
  bool initalized = false;
};
