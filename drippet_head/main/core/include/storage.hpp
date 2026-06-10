#include "config.hpp"
#include "constants.hpp"
#include "node.hpp"

using all_durations_t = std::array<Time::Time_Seconds, config::max_nodes>;
struct Storage {
  virtual ~Storage() = default;
  virtual Esp_Err_t save_durations(size_t addr,
                                   NodeTypes::HoseDuration duration,
                                   const NodeTypes::WateringCycle &cycle) = 0;

  virtual NodeTypes::DurationSchedule
  read_boot_durations(size_t addr) const = 0;
  virtual Esp_Err_t init() = 0;
};
