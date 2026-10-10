#pragma once

#include <ocr/BoundingBox.hpp>
#include <ocr/OcrLine.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr {

/**
 * @brief Represents a recognized block/paragraph of text consisting of lines.
 */
struct OcrBlock {
    std::string text;
    float confidence = 0.0f; // Normalized 0.0 to 1.0
    BoundingBox bbox;
    std::vector<OcrLine> lines;

    [[nodiscard]] nlohmann::json toJson(bool includeLines = true, bool includeTokens = true) const {
        nlohmann::json j{
            {"text", text},
            {"confidence", confidence},
            {"bbox", bbox.toJson()}
        };
        if (includeLines) {
            nlohmann::json linesJson = nlohmann::json::array();
            for (const auto& line : lines) {
                linesJson.push_back(line.toJson(includeTokens));
            }
            j["lines"] = std::move(linesJson);
        }
        return j;
    }

    static OcrBlock fromJson(const nlohmann::json& j) {
        OcrBlock block;
        block.text = j.value("text", "");
        block.confidence = j.value("confidence", 0.0f);
        if (j.contains("bbox")) {
            block.bbox = BoundingBox::fromJson(j["bbox"]);
        }
        if (j.contains("lines") && j["lines"].is_array()) {
            for (const auto& l : j["lines"]) {
                block.lines.push_back(OcrLine::fromJson(l));
            }
        }
        return block;
    }
};

} // namespace ocr
