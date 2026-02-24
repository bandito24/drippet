#include "logger.hpp"
#include "esp_log.h"
#include <string>
static const char *TAG = "Message: ";
void Logger::log_simple(const std::string &message) {

  ESP_LOGI(TAG, "%s", message.c_str());
}

void Logger::log_error(const std::string &message) {
  ESP_LOGE(TAG, "%s", message.c_str());
}
