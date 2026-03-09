#pragma once
#include <chrono>
#include <memory>

namespace Time {
using Time_Point = std::chrono::system_clock::time_point;
using Time_Seconds = std::chrono::seconds;
constexpr Time_Seconds No_Time = std::chrono::seconds{0};

constexpr Time_Seconds Day_In_Seconds = std::chrono::seconds{24 * 60 * 60};
struct WateringSchedule {
  uint8_t hour;
  uint8_t minute;
};

} // namespace Time
  //
struct iClock {
  virtual ~iClock() = default;
  virtual Time::Time_Point now() const = 0;
};
std::unique_ptr<iClock> initialize_clock();

class Esp32Clock : iClock {
public:
  Time::Time_Point now() const override;
  void setTime(int year, int month, int day, int hour, int min = 0,
               int second = 0);
  void setDailyWatering(uint8_t hour, uint8_t min);
  time_t last_watering_ts = 0;
  time_t next_watering_ts = 0;
  Time::WateringSchedule watering_schedule{};

private:
  bool initalized = false;
  time_t makeTime(int year, int month, int day, int hour, int min, int second);
};
