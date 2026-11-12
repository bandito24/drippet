#pragma once
#include "constants.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <sys/time.h>

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
  // True UTC. Add get_utc_offset_min() for local time.
  virtual Time::Time_Point now() const = 0;
  virtual Time::UtcOffsetMin get_utc_offset_min() const = 0;
  // False until the first successful Clock Sync (set_time)
  virtual bool is_synced() const = 0;
  virtual bool is_watering_due() const = 0;

  // Cycle Day: the local weekday, Sunday = CyclePhase::FIRST
  virtual CyclePhase get_phase_of_cycle() const = 0;
  virtual Time::Long get_phase_length() const = 0;

  // Local time of day
  virtual HourMin get_hourmin_curr_time() const = 0;
  // The stored local Watering Start Time (not the next watering point)
  virtual std::optional<HourMin> get_hourmin_next_phase() const = 0;
  virtual void set_next_phase_start_time(uint8_t hour, uint8_t min) = 0;

  // Clock Sync. Returns INVALID_TIME and leaves the clock untouched when the
  // timestamp or offset is out of range.
  virtual ActionStatus set_time(Time::Epoch utc_seconds,
                                Time::UtcOffsetMin offset_min) = 0;
  virtual void recalculate_next_watering_point() = 0;
};

class Esp32Clock : public iClock {
public:
  Esp32Clock(Time::Long _phase_length, iSysClock &_sys)
      : phase_length{_phase_length}, sys{_sys} {};
  Time::Time_Point now() const override;

  Time::UtcOffsetMin get_utc_offset_min() const override {
    return this->utc_offset_min;
  }
  bool is_synced() const override { return this->synced; }

  CyclePhase get_phase_of_cycle() const override {
    std::chrono::weekday day{
        std::chrono::floor<std::chrono::days>(this->to_local(this->now()))};
    return static_cast<CyclePhase>(day.c_encoding());
  };

  void recalculate_next_watering_point() override;
  ActionStatus set_time(Time::Epoch utc_seconds,
                        Time::UtcOffsetMin offset_min) override;
  void set_next_phase_start_time(uint8_t hour, uint8_t min) override;
  std::optional<Time::Time_Point> get_next_watering_point() {
    return this->next_watering_point;
  }

  HourMin get_hourmin_curr_time() const override;
  std::optional<HourMin> get_hourmin_next_phase() const override;

  Time::Long get_phase_length() const override { return this->phase_length; }
  bool is_watering_due() const override;

private:
  Time::UtcOffsetMin utc_offset_min{0};
  bool synced = false;
  std::optional<HourMin> phase_start_schedule = std::nullopt;
  std::optional<Time::Time_Point> next_watering_point = std::nullopt;
  Time::Long phase_length;
  // Time of day of an already-local time point
  static HourMin cast_hour_min(const Time::Time_Point &local_point);
  Time::Time_Point to_local(Time::Time_Point utc) const {
    return utc + std::chrono::minutes{this->utc_offset_min};
  }
  Time::Time_Point from_local(Time::Time_Point local) const {
    return local - std::chrono::minutes{this->utc_offset_min};
  }
  iSysClock &sys;
};
