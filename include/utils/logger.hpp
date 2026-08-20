#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <SDL3/SDL.h>
#include <filesystem>

// Log rotation
#include <cstdio>

// Time
#include <chrono>
#include <iomanip>

/////////////
// LOGGING //
/////////////

// Windows is stupid
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
    public:
        Logger() {
            start_time = std::chrono::steady_clock::now();
            previous_time = std::chrono::steady_clock::now();
        }

        ~Logger() {
            if (log_file.is_open()) {
                log_file.close();
            }
        }

        void log(const std::filesystem::path& file, const char* func, LogLevel level, const char* format, ...) {
            std::ostringstream oss;
                va_list args;
                va_start(args, format);
                
                char buffer[256];
                vsnprintf(buffer, sizeof(buffer), format, args);
                va_end(args);
            oss << buffer;

            std::string message = oss.str();

            if (!message.empty() && message.back() == '\n') {
                message.pop_back();
            }

            std::string prefix;
            std::string color_code;
            switch (level) {
                    case LogLevel::Debug:
                        prefix = "DEBUG   ";
                        color_code = "\033[32m"; // Green
                        break;
                    case LogLevel::Info:     
                        prefix = "INFO    ";
                        color_code = "\033[37m"; // White
                        break;
                    case LogLevel::Warning:
                        prefix = "WARNING ";
                        color_code = "\033[33m"; // Yellow
                        break;
                    case LogLevel::Error:
                        prefix = "ERROR   ";
                        color_code = "\033[31m"; // Red
                        break;
                    case LogLevel::Critical:
                        prefix = "CRITICAL";
                        color_code = "\033[41;37m"; // Red background with white text
                        break;
            }

            // Get elapsed time
            auto now = std::chrono::steady_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - previous_time).count();
            previous_time = now;

            // Convert elapsed time into seconds and milliseconds
            long seconds = elapsed_ms / 1000;
            long milliseconds = elapsed_ms % 1000;

            std::ostringstream timeStream;
            timeStream << seconds << '.' << std::setfill('0') << std::setw(3) << milliseconds;

            std::string timestamp = timeStream.str();
            std::string full_log = prefix + ": [" + timestamp + "]" + "[" + file.filename().u8string() + ":" + std::string(func) + "] " + message;
            std::string full_log_colour = color_code + prefix + "\033[0m" + ": [" + timestamp + "]" + "[" + file.filename().u8string() + ":" + std::string(func) + "] " + message;

            if (log_file.is_open()) {
                log_file << full_log << std::endl;  
            }
            SDL_Log("%s", full_log_colour.c_str());
        }

        bool set_logfile(const std::filesystem::path& filename) {
            const std::string filename_str = filename.u8string();
            SDL_RemovePath(filename_str.c_str());
            log_file.open(filename, std::ios::out | std::ios::app);
            if (!log_file.is_open()) {
                SDL_Log("\033[31mERROR\033[0m: Could not open log file: %s", filename_str.c_str());
                return false;
            }
            log_file << "--- Cleared and Set log file to " << filename_str << " ---"<< std::endl;
            return true;
        }

    private:
        std::ofstream log_file;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point previous_time;
};

inline Logger LOGGER;
#define LOG(level, format, ...) LOGGER.log(__FILE__, __func__, level, format, ##__VA_ARGS__)

#endif