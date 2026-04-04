#include "logger.hpp"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <vector>

std::string format(const char *fmt, va_list args);
void Logger::log_simple(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::cout << "[INFO]: " << format(fmt, args) << std::endl;
  va_end(args);
}

void Logger::log_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::cerr << "[ERROR]: " << format(fmt, args) << std::endl;
  va_end(args);
}
std::string format(const char *fmt, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(nullptr, 0, fmt, args_copy);
  va_end(args_copy);
  std::vector<char> buffer(size + 1);
  vsnprintf(buffer.data(), buffer.size(), fmt, args);
  return std::string(buffer.data());
}
