#include "time.hpp"

// FOR Host computer with development
class Clock : public iClock {
public:
  static Time::Time_Point static_now() {
    return std::chrono::system_clock::now();
  }
  Time::Time_Point now() const override { return Clock::static_now(); };
};

std::unique_ptr<iClock> initialize_clock() { return std::make_unique<Clock>(); }
