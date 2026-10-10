#pragma once

#include <edmo/ocr/BoundingBox.hpp>
#include <edmo/ocr/OcrToken.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::ocr {

/**
 * @brief Represents a recognized line of text consisting of one or more tokens.
 */
struct OcrLine {
    std::string text;
    float confidence = 0.0f; // Normalized 0.0 to 1.0
    BoundingBox bbox;
    std::vector<OcrToken> tokens;

    [[nodiscard]] nlohmann::json toJson(bool includeTokens = true) const {
        nlohmann::json j{
            {"text", text},
            {"confidence", confidence},
            {"bbox", bbox.toJson()}
        };
        if (includeTokens) {
            nlohmann::json tokensJson = nlohmann::json::array();
            for (const auto& token : tokens) {
                tokensJson.push_back(token.toJson());
            }
            j["tokens"] = std::move(tokensJson);
        }
        return j;
    }

    static OcrLine fromJson(const nlohmann::json& j) {
        OcrLine line;
        line.text = j.value("text", "");
        line.confidence = j.value("confidence", 0.0f);
        if (j.contains("bbox")) {
            line.bbox = BoundingBox::fromJson(j["bbox"]);
        }
        if (j.contains("tokens") && j["tokens"].is_array()) {
            for (const auto& t : j["tokens"]) {
                line.tokens.push_back(OcrToken::fromJson(t));
            }
        }
        return line;
    }
};

} // namespace edmo::ocr
