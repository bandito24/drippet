#pragma once
#include "config.hpp"
#include "constants.hpp"
#include <cassert>
#include <memory>
struct Switch {
  virtual Esp_Err_t enable() = 0;
  virtual Esp_Err_t init() = 0;
  virtual Esp_Err_t disable() = 0;
  virtual bool is_enabled() const = 0;
  virtual ~Switch() = default;
};
class SolenoidValve : public Switch {};

using SolenoidGrouping =
    std::array<std::unique_ptr<SolenoidValve>, config::node_hose_count>;

class SolenoidManager {
public:
  Esp_Err_t initialize_solenoids() {
    Esp_Err_t rc{};
    for (auto &valve : solenoids) {
      assert(valve);
      rc |= valve->init();
    }
    return rc;
  };
  Esp_Err_t activate_solenoid(size_t index) {
    assert(this->solenoids.at(index));
    this->deactivate_solenoids();
    return this->solenoids.at(index)->enable();
  };
  Esp_Err_t deactivate_solenoids() {

    Esp_Err_t rc{};
    for (auto &valve : solenoids) {
      valve->disable();
    }
    return rc;
  }
  size_t active_index() {
    for (size_t i = 0; i < this->solenoids.size(); i++) {
      if (this->solenoids.at(i)->is_enabled()) {
        return i;
      }
    }
    return HOSE_INACTIVE_IDX;
  }
  SolenoidManager(SolenoidGrouping valves) : solenoids{std::move(valves)} {};
  ~SolenoidManager() { this->deactivate_solenoids(); }

private:
  SolenoidGrouping solenoids{};
};
