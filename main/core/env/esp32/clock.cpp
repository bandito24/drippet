#include "clock.hpp"
#include <chrono>
using namespace std::chrono;

time_t Esp32Clock::makeTime(int year, int month, int day, int hour, int min,
                            int second) {
  struct tm tm;
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_sec = second;
  return mktime(&tm);
}

constexpr uint8_t MINS_BUFFER = 2;

// Zero defaults on min and second
void Esp32Clock::setTime(int year, int month, int day, int hour, int min,
                         int second) {
  time_t t = this->makeTime(year, month, day, hour, min, second);
  struct timeval now = {.tv_sec = t, .tv_usec = 0};
  settimeofday(&now, NULL);
}
Time::Time_Point Esp32Clock::now() const {
  const time_point<system_clock> now = system_clock::now();
  return now;
}
void Esp32Clock::setDailyWatering(uint8_t hour, uint8_t min) {
  this->watering_schedule = {.hour = hour, .minute = min};
  system_clock::time_point time_point =
      system_clock::from_time_t(this->next_watering_ts);
  struct tm timeinfo;
  if (this->next_watering_ts >
      system_clock::to_time_t(this->now() + minutes{MINS_BUFFER})) {
    // Means the next watering will be done in
    // more than ten minutes from now. Adjust it
    // to take place at that time
  }

  localtime_r(&t, &timeinfo);

  if (t > (this->now() + std::chrono::minutes{10}))
}
