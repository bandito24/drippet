#include "time.hpp"
// For Esp32 Production
class Esp32Clock : public iClock {
public:
  static Time::Time_Point static_now() {
    return std::chrono::system_clock::now();
  }
  Time::Time_Point now() const override { return Esp32Clock::static_now(); };
};

std::unique_ptr<iClock> initialize_clock() {
  return std::make_unique<Esp32Clock>();
}
