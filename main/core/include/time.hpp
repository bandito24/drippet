#pragma once
#include <chrono>
#include <memory>

namespace Time {
using Time_Point = std::chrono::system_clock::time_point;
using Time_Seconds = std::chrono::seconds;
constexpr Time_Seconds No_Time = std::chrono::seconds{0};

constexpr Time_Seconds Day_In_Seconds = std::chrono::seconds{24 * 60 * 60};

} // namespace Time
  //
struct iClock {
  virtual ~iClock() = default;
  virtual Time::Time_Point now() const = 0;
};
std::unique_ptr<iClock> initialize_clock();
