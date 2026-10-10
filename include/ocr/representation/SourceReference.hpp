#pragma once

#include <ocr/BoundingBox.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::representation {

/**
 * @brief Traceability metadata linking an EDR semantic entity back to original OCR tokens / page geometry.
 */
struct SourceReference {
    int pageNumber = 1;
    BoundingBox bbox;
    std::vector<std::string> ocrIds;

    [[nodiscard]] bool empty() const noexcept {
        return pageNumber <= 0 && bbox.empty() && ocrIds.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeOcrIds = true) const {
        nlohmann::json j{
            {"page", pageNumber},
            {"bbox", bbox.toJson()}
        };
        if (includeOcrIds && !ocrIds.empty()) {
            j["ocr_ids"] = ocrIds;
        }
        return j;
    }

    static SourceReference fromJson(const nlohmann::json& j) {
        SourceReference ref;
        ref.pageNumber = j.value("page", 1);
        if (j.contains("bbox")) {
            ref.bbox = BoundingBox::fromJson(j["bbox"]);
        }
        if (j.contains("ocr_ids") && j["ocr_ids"].is_array()) {
            for (const auto& id : j["ocr_ids"]) {
                ref.ocrIds.push_back(id.get<std::string>());
            }
        }
        return ref;
    }
};

} // namespace ocr::representation
