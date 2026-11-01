#pragma once

#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

class Logger {
   public:
    enum Type { DEBUG, SUCCESS, INFO, WARNING, ERROR };

   public:
    Logger();
    ~Logger() = default;

    void enable_log_file(const std::string& path);
    void enable_log_shell();

    template <typename... targs>
    void log(Type type, const targs&... args) {
        std::lock_guard lock(mutex);

        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
        localtime_r(&time, &tm);

        std::ostringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        std::string date = ss.str();

        (ss << ... << args);
        std::string msg = date + std::string(" [") + type_msgs[type] +
                          std::string("] -> ") + ss.str();

        if (in_file) {
            file << msg << "\n";
            file.flush();
        } else if (in_shell) {
            std::cout << msg << std::endl;
        }
    }

   private:
    bool in_file;
    bool in_shell;

    std::ofstream file;
    std::mutex    mutex;

    std::array<const char*, 5> type_msgs;
};
