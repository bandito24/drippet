#include "config.hpp"
#include "constants.hpp"

using all_durations_t =
    std::array<std::array<Time::Time_Seconds, config::node_hose_count>,
               config::max_nodes>;
struct Storage {
  virtual ~Storage() = default;
  virtual Esp_Err_t save_durations(all_durations_t &arg) = 0;
  virtual all_durations_t read_durations() const = 0;
  virtual Esp_Err_t init() = 0;
};
