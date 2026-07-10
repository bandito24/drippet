#include "nvs_storage.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "esp_err.h"
#include "head.hpp"
#include "logger.hpp"
#include "node.hpp"
#include "nvs_flash.h"
#include "util.hpp"
#include <cstring>

Esp_Err_t NvsStorage::init() {

  Esp_Err_t err{};

  err = nvs_flash_init();

  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NO_MEM) {
    nvs_flash_erase();
    err = nvs_flash_init();
    ESP_ERROR_CHECK(err);
  }
  this->handle = nvs::open_nvs_handle(handle_name, NVS_READWRITE, &err);
  if (err != ESP_OK) {
    Logger::log_error("NVS Storage Failed With: %s", esp_err_to_name(err));
  } else {
    this->initialized = true;
    this->load_durations();
  }

  // this->print_boot_persisted_durations();
  return err;
}
Esp_Err_t NvsStorage::load_durations() {
  Esp_Err_t rc;
  rc = handle->get_blob(this->handle_name, &this->boot_persisted_durations,
                        sizeof(this->boot_persisted_durations));

  if (rc == ESP_ERR_NVS_NOT_FOUND) {
    Logger::log_simple(this->handle_name, "No existing data");
  } else if (rc != ESP_OK) {
    Logger::log_error(this->handle_name, "Error (%s) reading of array!",
                      esp_err_to_name(rc));
  } else {
    Logger::log_simple("Array values exist!");
  }
  return rc;
}
NodeTypes::DurationSchedule NvsStorage::read_boot_durations(size_t addr) const {
  if (!this->initialized) {
    Logger::log_error("Storage not properly initalized");
    return {};
  }
  NodeTypes::DurationSchedule durations_sch{
      this->boot_persisted_durations[addr]};

  return durations_sch;
}
Esp_Err_t NvsStorage::save_durations(size_t addr,
                                     const NodeTypes::HoseDuration duration,
                                     const NodeTypes::WateringCycle &cycle) {
  Esp_Err_t rc{};
  if (!this->initialized) {
    Logger::log_error("Storage not properly initalized");
  }

  assert(addr < config::max_nodes);

  this->boot_persisted_durations[addr] = {.duration = duration, .cycle = cycle};

  rc = handle->set_blob(this->handle_name, &this->boot_persisted_durations,
                        sizeof(this->boot_persisted_durations));

  if (rc != ESP_OK) {
    Logger::log_error(this->handle_name, "Error (%s) adding to array!",
                      esp_err_to_name(rc));
    return rc;
  }
  rc = handle->commit();
  if (rc != ESP_OK) {
    Logger::log_error(this->handle_name, "Error (%s) committing data!",
                      esp_err_to_name(rc));
  }
  return rc;
}
void NvsStorage::print_boot_persisted_durations() const {
  for (size_t node = 0; node < config::max_nodes; ++node) {
    printf("Durations for Node %2u | ", (unsigned)node);

    printf("%5u ", (unsigned)this->boot_persisted_durations[node].duration);

    printf("\n");

    printf("Schedule for Node %2u | ", (unsigned)node);

    for (size_t hose = 0; hose < days_in_week; ++hose) {
      printf("%5u ",
             (unsigned)this->boot_persisted_durations[node].cycle[hose]);
    }

    printf("\n");
  }
}
