#pragma once

#include <ocr/representation/SemanticDocument.hpp>

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace ocr::representation {

class EdrJsonSerializer {
public:
    static nlohmann::json toFullJson(
        const SemanticDocument& doc
    );

    static nlohmann::json toLlmJson(
        const SemanticDocument& doc,
        bool includeSource = false
    );

    static std::string serializeFull(
        const SemanticDocument& doc,
        bool prettyPrint = true
    );

    static std::string serializeLlm(
        const SemanticDocument& doc,
        bool includeSource = false,
        bool prettyPrint = true
    );

    static bool saveToFile(
        const std::filesystem::path& path,
        const nlohmann::json& json,
        bool prettyPrint = true
    );
};

} // namespace ocr::representation
