#pragma once

#include <ocr/BoundingBox.hpp>
#include <ocr/OcrLine.hpp>
#include <ocr/representation/ConfidenceSummary.hpp>
#include <ocr/representation/DocumentRepresentation.hpp>
#include <ocr/representation/SourceReference.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::representation {

/**
 * @brief Represents a reconstructed semantic block of text (Heading, Paragraph, LabelValue, etc.)
 */
struct RepresentationBlock {
    std::string id;
    std::string text;
    std::string normalizedText;
    BlockType type = BlockType::Unknown;

    // For LabelValue blocks (e.g. "NAME : Sophia Jones" -> label: "NAME", value: "Sophia Jones")
    std::string label;
    std::string value;

    BoundingBox bbox;
    ConfidenceSummary confidence;
    SourceReference source;
    std::vector<OcrLine> lines;

    [[nodiscard]] bool empty() const noexcept {
        return text.empty() && normalizedText.empty() && label.empty() && value.empty() && lines.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true, bool includeLines = false) const {
        nlohmann::json j{
            {"id", id},
            {"type", blockTypeToString(type)},
            {"text", normalizedText.empty() ? text : normalizedText},
            {"confidence", confidence.toJson()}
        };

        if (type == BlockType::LabelValue) {
            j["label"] = label;
            j["value"] = value;
        }

        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }

        if (includeLines && !lines.empty()) {
            nlohmann::json linesJson = nlohmann::json::array();
            for (const auto& l : lines) {
                linesJson.push_back(l.toJson(false));
            }
            j["lines"] = std::move(linesJson);
        }

        return j;
    }
};

} // namespace ocr::representation
