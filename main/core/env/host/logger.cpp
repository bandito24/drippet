#include "logger.hpp"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

std::string format(const char *fmt, va_list args);
// ✅ Existing (backwards compatible)
void Logger::log_simple(const std::string &message) {
  std::cout << message << std::endl;
}

void Logger::log_error(const std::string &message) {
  std::cerr << message << std::endl;
}

// ✅ New (printf-style)
void Logger::log_simple(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  std::string msg = format(fmt, args);

  va_end(args);

  std::cout << "[INFO]: " << msg << std::endl;
}

void Logger::log_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  std::string msg = format(fmt, args);

  va_end(args);

  std::cerr << "[ERROR]: " << msg << std::endl;
}

// 🔥 Helper to format variadic args safely
std::string format(const char *fmt, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);

  int size = vsnprintf(nullptr, 0, fmt, args_copy);
  va_end(args_copy);

  std::vector<char> buffer(size + 1);
  vsnprintf(buffer.data(), buffer.size(), fmt, args);

  return std::string(buffer.data());
}
