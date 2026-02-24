#include <string>
class Logger {
public:
  static void log_simple(const std::string &message);
  static void log_error(const std::string &message);
};
