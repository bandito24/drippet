#include "nvs_storage.hpp"
#include "constants.hpp"
#include "esp_err.h"
#include "logger.hpp"

Esp_Err_t NvsStorage::init() {

  Esp_Err_t err{};
  this->handle = nvs::open_nvs_handle(handle_name, NVS_READWRITE, &err);
  if (err != ESP_OK) {
    Logger::log_error("NVS Storage Failed With: %s", esp_err_to_name(err));
  }
  return err;
}
Esp_Err_t NvsStorage::save_durations(all_durations_t &arg) {}
