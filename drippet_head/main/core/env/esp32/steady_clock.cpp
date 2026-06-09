#include "steady_clock.hpp"
#include <chrono>

Time::Long SteadyEspClock::now() const {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
