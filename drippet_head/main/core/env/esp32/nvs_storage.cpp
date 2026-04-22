#include "nvs_storage.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "esp_err.h"
#include "head.hpp"
#include "node.hpp"
#include "nvs_flash.h"
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

  this->print_boot_persisted_durations();
  return err;
}
Esp_Err_t NvsStorage::load_durations() {
  Esp_Err_t rc;
  rc = handle->get_blob(this->handle_name, this->boot_persisted_durations,
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
NodeTypes::HoseDurations NvsStorage::read_boot_durations(size_t addr) const {
  if (!this->initialized) {
    Logger::log_error("Storage not properly initalized");
  }
  NodeTypes::HoseDurations durations{};
  memcpy(durations.data(), this->boot_persisted_durations[addr],
         sizeof(this->boot_persisted_durations[addr]));
  return durations;
}
Esp_Err_t NvsStorage::save_durations(size_t addr,
                                     NodeTypes::HoseDurations &durations) {
  Esp_Err_t rc{};
  if (!this->initialized) {
    Logger::log_error("Storage not properly initalized");
  }

  assert(addr < config::max_nodes);
  memcpy(this->boot_persisted_durations[addr], durations.data(),
         sizeof(this->boot_persisted_durations[addr]));

  rc = handle->set_blob(this->handle_name, this->boot_persisted_durations,
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
    printf("Node %2u | ", (unsigned)node);

    for (size_t hose = 0; hose < config::node_hose_count; ++hose) {
      printf("%5u ", (unsigned)this->boot_persisted_durations[node][hose]);
    }

    printf("\n");
  }
}
