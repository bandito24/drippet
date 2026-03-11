#include "clock.hpp"
#include <chrono>
#include <ctime>
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

constexpr int SECONDS_PER_DAY = 24 * 60 * 60;

// Zero defaults on min and second
time_t Esp32Clock::set_time(int year, int month, int day, int hour, int min,
                            int second) {
  time_t t = Esp32Clock::makeTime(year, month, day, hour, min, second);
  struct timeval now = {.tv_sec = t, .tv_usec = 0};
  this->sys.change_system_clock(now);
  this->initalized = true;
  if (this->next_watering_ts && this->next_watering_ts < t) {
    this->next_watering_ts += SECONDS_PER_DAY;
  }
  return t;
}
Time::Time_Point SystemClock::now() const {
  const time_point<system_clock> now = system_clock::now();
  return now;
}
void SystemClock::change_system_clock(const timeval &time) {
  settimeofday(&time, NULL);
}
Time::Time_Point Esp32Clock::now() const { return this->sys.now(); }

time_t Esp32Clock::set_daily_watering(uint8_t hour, uint8_t min) {
  this->watering_schedule = {.hour = hour, .minute = min};

  time_t now_time = system_clock::to_time_t(this->now());

  tm timeinfo;
  localtime_r(&now_time, &timeinfo);

  timeinfo.tm_hour = hour;
  timeinfo.tm_min = min;
  timeinfo.tm_sec = 0;

  time_t next_watering_ts = mktime(&timeinfo);

  if (next_watering_ts <= now_time) {
    next_watering_ts += SECONDS_PER_DAY;
  }

  this->next_watering_ts = next_watering_ts;
  return next_watering_ts;
}
