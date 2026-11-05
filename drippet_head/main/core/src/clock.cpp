#include "clock.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include <chrono>
#include <ctime>
#include <optional>
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
ActionStatus Esp32Clock::set_time(time_t time_point, int offsetMin) {

  if (offsetMin > Time::OFFSET_RANGES[1] ||
      offsetMin < Time::OFFSET_RANGES[0] ||
      time_point < Time::MIN_VALID_EPOCH) {
    return ActionStatus::INVALID_TIME;
  }

  timeval now = {.tv_sec = time_point, .tv_usec = 0};
  this->sys.change_system_clock(now);
  this->UTC_OFFSET_MIN = offsetMin;
  this->recalculate_next_watering_point();
  this->synced = true;
  return ActionStatus::OK;
}

HourMin Esp32Clock::get_hourmin_curr_time() const {
  return this->cast_hour_min(this->now() + minutes{this->UTC_OFFSET_MIN});
};

std::optional<HourMin> Esp32Clock::get_hourmin_next_phase() const {
  if (this->next_watering_point) {
    return this->cast_hour_min(*this->next_watering_point +
                               minutes{this->UTC_OFFSET_MIN});
  }
  return std::nullopt;
}
HourMin Esp32Clock::cast_hour_min(const Time::Time_Point &time_point) const {

  std::chrono::hh_mm_ss time_of_day{
      time_point - std::chrono::floor<std::chrono::days>(time_point)};
  return {.hour = static_cast<uint8_t>(time_of_day.hours().count()),
          .minute = static_cast<uint8_t>(time_of_day.minutes().count())};
}

Time::Time_Point SystemClock::now() const {
  const time_point<system_clock> now = system_clock::now();
  return now;
}
void SystemClock::change_system_clock(const timeval &time) {
  settimeofday(&time, NULL);
}
Time::Time_Point Esp32Clock::now() const { return this->sys.now(); }

void Esp32Clock::set_next_phase_start_time(uint8_t hour, uint8_t min) {
  this->phase_start_schedule = {hour, min};
  this->recalculate_next_watering_point();
}
void Esp32Clock::recalculate_next_watering_point() {
  if (!this->phase_start_schedule) {
    this->next_watering_point = std::nullopt;
    return;
  }
  int hour = this->phase_start_schedule->hour;
  int min = this->phase_start_schedule->minute;

  Time::Time_Point current_time = this->now() + minutes{this->UTC_OFFSET_MIN};
  Time::Time_Point candidate =
      floor<days>(current_time) + hours{hour} + minutes{min};

  while (current_time >= candidate) {
    candidate += seconds{this->phase_length};
  }
  this->next_watering_point = candidate - minutes{this->UTC_OFFSET_MIN};
}
bool Esp32Clock::is_watering_due() const {
  if (!this->next_watering_point || !this->synced) {
    return false;
  }

  return this->now() >= *this->next_watering_point;
}
