#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

class Logger {
public:
  // ✅ Existing (backwards compatible)
  static void log_simple(const std::string &message) {
    std::cout << message << std::endl;
  }

  static void log_error(const std::string &message) {
    std::cerr << message << std::endl;
  }

  // ✅ New (printf-style)
  static void log_simple(const char *fmt, ...) {
    std::string msg = format(fmt);
    std::cout << "[INFO]: " << msg << std::endl;
  }

  static void log_error(const char *fmt, ...) {
    std::string msg = format(fmt);
    std::cerr << "[ERROR]: " << msg << std::endl;
  }

private:
  // 🔥 Helper to format variadic args safely
  static std::string format(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // First pass: get required size
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    std::vector<char> buffer(size + 1);
    vsnprintf(buffer.data(), buffer.size(), fmt, args);

    va_end(args);

    return std::string(buffer.data());
  }
};
