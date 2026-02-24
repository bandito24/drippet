#include "logger.hpp"
#include <iostream>
#include <string>
void Logger::log_simple(const std::string &message) {
  std::cout << message << std::endl;
}
