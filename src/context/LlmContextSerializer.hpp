#pragma once

#include <edmo/ocr/context/LlmContextConfig.hpp>
#include <edmo/ocr/context/LlmContextModels.hpp>
#include <edmo/ocr/context/LlmContextReport.hpp>

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace edmo::ocr::context {

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

} // namespace edmo::ocr::context
