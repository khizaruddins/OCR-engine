#pragma once

#include <edmo/ocr/BoundingBox.hpp>

#include <string>
#include <nlohmann/json.hpp>

namespace edmo::ocr {

/**
 * @brief Represents an individual OCR token (word / symbol).
 */
struct OcrToken {
    std::string text;
    float confidence = 0.0f; // Normalized 0.0 to 1.0
    BoundingBox bbox;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"text", text},
            {"confidence", confidence},
            {"bbox", bbox.toJson()}
        };
    }

    static OcrToken fromJson(const nlohmann::json& j) {
        OcrToken token;
        token.text = j.value("text", "");
        token.confidence = j.value("confidence", 0.0f);
        if (j.contains("bbox")) {
            token.bbox = BoundingBox::fromJson(j["bbox"]);
        }
        return token;
    }
};

} // namespace edmo::ocr
