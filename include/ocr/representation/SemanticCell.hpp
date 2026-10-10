#pragma once

#include <ocr/representation/SourceReference.hpp>

#include <string>
#include <nlohmann/json.hpp>

namespace ocr::representation {

/**
 * @brief Represents an individual cell within a SemanticRow / SemanticTable.
 */
struct SemanticCell {
    std::string rawText;
    std::string normalizedText;
    float confidence = 0.0f;
    SourceReference source;
    int colSpan = 1;
    int rowSpan = 1;

    [[nodiscard]] bool empty() const noexcept {
        return rawText.empty() && normalizedText.empty();
    }

    [[nodiscard]] const std::string& text() const noexcept {
        return normalizedText.empty() ? rawText : normalizedText;
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"text", text()},
            {"confidence", confidence}
        };
        if (colSpan > 1) j["colspan"] = colSpan;
        if (rowSpan > 1) j["rowspan"] = rowSpan;
        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }
        return j;
    }
};

} // namespace ocr::representation
