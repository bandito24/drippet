#include "constants.hpp"

struct SteadyClock {
  virtual Time::Long now() const = 0;
};
struct SteadyEspClock : public SteadyClock {
  Time::Long now() const override;
};
