#pragma once
#include "constants.hpp"
#include "head.hpp"
#include "node.hpp"
#include "nvs_handle.hpp"

using handle_t = std::unique_ptr<nvs::NVSHandle>;

class NvsStorage : public Storage {
public:
  Esp_Err_t save_durations(size_t addr,
                           NodeTypes::HoseDurations &durations) override;
  NodeTypes::HoseDurations read_boot_durations(size_t addr) const override;
  Esp_Err_t init() override;
  void print_boot_persisted_durations() const;

private:
  Esp_Err_t load_durations();
  const char *const handle_name{"storage"};
  handle_t handle{};
  Time::Time_Seconds boot_persisted_durations[config::max_nodes]
                                             [config::node_hose_count] = {};
  bool initialized = false;
};
