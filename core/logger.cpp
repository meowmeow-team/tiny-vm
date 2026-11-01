#include "logger.hpp"

Logger::Logger()
    : in_file(false),
      in_shell(false),
      mutex(),
      type_msgs{"DEBUG", "SUCCESS", "INFO", "WARNING", "ERROR"} {}

void Logger::enable_log_file(const std::string& path) {
    if (path.empty()) throw std::logic_error("path for log file is empty");

    if (file.is_open()) file.close();

    file = std::ofstream(path, std::ios::app);
    if (!file.is_open())
        throw std::runtime_error("failed to open the log fiel");

    in_file = true;
}

void Logger::enable_log_shell() { in_shell = true; }
