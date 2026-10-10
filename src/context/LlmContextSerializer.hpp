#pragma once

#include <ocr/context/LlmContextConfig.hpp>
#include <ocr/context/LlmContextModels.hpp>
#include <ocr/context/LlmContextReport.hpp>

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace ocr::context {

class LlmContextSerializer {
public:
    static nlohmann::json toJson(
        const LlmContext& context,
        ContextMode mode = ContextMode::Compact
    );

    static nlohmann::json toReportJson(
        const LlmContextReport& report
    );

    static bool saveToFile(
        const std::filesystem::path& path,
        const nlohmann::json& json,
        bool prettyPrint = true
    );
};

} // namespace ocr::context
