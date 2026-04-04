#include "logger.hpp"
#include "esp_log.h"
#include <string>
static const char *TAG = "[INFO]: ";

static const char *TAGERR = "[ERROR]: ";
void Logger::log_simple(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  ESP_LOGI(TAG, "%s", buf); // let the macro handle tag/formatting
}

void Logger::log_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  ESP_LOGE(TAG, "%s", buf);
}
