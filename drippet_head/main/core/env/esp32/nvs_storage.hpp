#pragma once
#include "config.hpp"
#include "constants.hpp"
#include "head.hpp"
#include "node.hpp"
#include "nvs_handle.hpp"

using handle_t = std::unique_ptr<nvs::NVSHandle>;
using DurationScheduleAll =
    std::array<NodeTypes::DurationSchedule, config::max_nodes>;

class NvsStorage : public Storage {
public:
  Esp_Err_t
  save_durations(size_t addr, const NodeTypes::HoseDurations &durations,
                 const NodeTypes::WateringCycle &schedule) override;
  NodeTypes::DurationSchedule read_boot_durations(size_t addr) const override;

  Esp_Err_t init() override;
  void print_boot_persisted_durations() const;

private:
  Esp_Err_t load_durations();
  const char *const handle_name{"storage"};
  handle_t handle{};

  DurationScheduleAll boot_persisted_durations = {};
  bool initialized = false;
};
