#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace edmo::ocr {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

class Logger {
public:
    static void init(LogLevel level = LogLevel::Info, const std::string& logFile = "");
    static void setLevel(LogLevel level);
    static std::shared_ptr<spdlog::logger>& getLogger();
};

#define EDMO_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::edmo::ocr::Logger::getLogger(), __VA_ARGS__)
#define EDMO_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(::edmo::ocr::Logger::getLogger(), __VA_ARGS__)
#define EDMO_LOG_INFO(...)  SPDLOG_LOGGER_INFO(::edmo::ocr::Logger::getLogger(), __VA_ARGS__)
#define EDMO_LOG_WARN(...)  SPDLOG_LOGGER_WARN(::edmo::ocr::Logger::getLogger(), __VA_ARGS__)
#define EDMO_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::edmo::ocr::Logger::getLogger(), __VA_ARGS__)
#define EDMO_LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::edmo::ocr::Logger::getLogger(), __VA_ARGS__)

} // namespace edmo::ocr
