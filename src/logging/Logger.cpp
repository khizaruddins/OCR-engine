#include "Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

namespace edmo::ocr {

static std::shared_ptr<spdlog::logger> s_logger;

void Logger::init(LogLevel level, const std::string& logFile) {
    if (spdlog::get("edmo-ocr")) {
        spdlog::drop("edmo-ocr");
    }
    std::vector<spdlog::sink_ptr> sinks;
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    sinks.push_back(console_sink);

    if (!logFile.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        sinks.push_back(file_sink);
    }

    s_logger = std::make_shared<spdlog::logger>("edmo-ocr", sinks.begin(), sinks.end());
    spdlog::register_logger(s_logger);
    setLevel(level);
}

void Logger::setLevel(LogLevel level) {
    if (!s_logger) {
        init(level);
        return;
    }

    switch (level) {
        case LogLevel::Trace:
            s_logger->set_level(spdlog::level::trace);
            break;
        case LogLevel::Debug:
            s_logger->set_level(spdlog::level::debug);
            break;
        case LogLevel::Info:
            s_logger->set_level(spdlog::level::info);
            break;
        case LogLevel::Warn:
            s_logger->set_level(spdlog::level::warn);
            break;
        case LogLevel::Error:
            s_logger->set_level(spdlog::level::err);
            break;
        case LogLevel::Critical:
            s_logger->set_level(spdlog::level::critical);
            break;
        case LogLevel::Off:
            s_logger->set_level(spdlog::level::off);
            break;
    }
}

std::shared_ptr<spdlog::logger>& Logger::getLogger() {
    if (!s_logger) {
        init(LogLevel::Info);
    }
    return s_logger;
}

} // namespace edmo::ocr
