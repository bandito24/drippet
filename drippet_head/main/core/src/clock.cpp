#include "clock.hpp"
#include "constants.hpp"
#include <chrono>
#include <optional>
using namespace std::chrono;

ActionStatus Esp32Clock::set_time(Time::Epoch utc_seconds,
                                  Time::UtcOffsetMin offset_min) {

  if (offset_min > Time::MAX_UTC_OFFSET_MIN ||
      offset_min < Time::MIN_UTC_OFFSET_MIN ||
      utc_seconds < Time::MIN_VALID_EPOCH) {
    return ActionStatus::INVALID_TIME;
  }

  timeval now = {.tv_sec = static_cast<time_t>(utc_seconds), .tv_usec = 0};
  this->sys.change_system_clock(now);
  this->utc_offset_min = offset_min;
  this->recalculate_next_watering_point();
  this->synced = true;
  return ActionStatus::OK;
}

HourMin Esp32Clock::get_hourmin_curr_time() const {
  return Esp32Clock::cast_hour_min(this->to_local(this->now()));
};

std::optional<HourMin> Esp32Clock::get_hourmin_next_phase() const {
  return this->phase_start_schedule;
}

HourMin Esp32Clock::cast_hour_min(const Time::Time_Point &local_point) {
  hh_mm_ss time_of_day{local_point - floor<days>(local_point)};
  return {.hour = static_cast<uint8_t>(time_of_day.hours().count()),
          .minute = static_cast<uint8_t>(time_of_day.minutes().count())};
}

Time::Time_Point SystemClock::now() const { return system_clock::now(); }
void SystemClock::change_system_clock(const timeval &time) {
  settimeofday(&time, NULL);
}
Time::Time_Point Esp32Clock::now() const { return this->sys.now(); }

void Esp32Clock::set_next_phase_start_time(uint8_t hour, uint8_t min) {
  this->phase_start_schedule = HourMin{static_cast<uint8_t>(hour % 24),
                                       static_cast<uint8_t>(min % 60)};
  this->recalculate_next_watering_point();
}

// Next watering = the first Watering Start Time (local) strictly after now,
// stepping by phase_length from today's start time. Works whether the clock
// moved forwards or backwards.
void Esp32Clock::recalculate_next_watering_point() {
  if (!this->phase_start_schedule) {
    this->next_watering_point = std::nullopt;
    return;
  }
  const Time::Time_Point local_now = this->to_local(this->now());
  Time::Time_Point candidate = floor<days>(local_now) +
                               hours{this->phase_start_schedule->hour} +
                               minutes{this->phase_start_schedule->minute};

  while (candidate <= local_now) {
    candidate += seconds{this->phase_length};
  }
  this->next_watering_point = this->from_local(candidate);
}

bool Esp32Clock::is_watering_due() const {
  if (!this->next_watering_point || !this->synced) {
    return false;
  }
  return this->now() >= *this->next_watering_point;
}
