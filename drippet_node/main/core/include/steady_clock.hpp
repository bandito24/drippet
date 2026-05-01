#include "constants.hpp"

struct Clock {
  virtual Time::Long now() const = 0;
};
struct SteadyEspClock : public Clock {
  Time::Long now() const override;
};
