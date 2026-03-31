#include "logger.hpp"
#include "esp_log.h"
#include <string>
static const char *TAG = "[INFO]: ";

static const char *TAGERR = "[ERROR]: ";
void Logger::log_simple(const std::string &message) {

  ESP_LOGI(TAG, "%s", message.c_str());
}

void Logger::log_error(const std::string &message) {
  ESP_LOGE(TAGERR, "%s", message.c_str());
}
void Logger::log_simple(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  esp_log_writev(ESP_LOG_INFO, TAG, fmt, args);

  va_end(args);
}

void Logger::log_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  esp_log_writev(ESP_LOG_ERROR, TAGERR, fmt, args);

  va_end(args);
}
