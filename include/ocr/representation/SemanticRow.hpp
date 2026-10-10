#pragma once

#include <edmo/ocr/representation/SemanticCell.hpp>
#include <edmo/ocr/representation/SourceReference.hpp>

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::ocr::representation {

/**
 * @brief Represents a row in a SemanticTable with cells and canonical field mappings.
 */
struct SemanticRow {
    std::vector<SemanticCell> cells;
    bool isHeader = false;
    float confidence = 0.0f;
    SourceReference source;

    // Canonical fields for domain records (e.g. course_code -> "CS101", credits -> "3", grade -> "A")
    std::map<std::string, std::string> canonicalFields;

    [[nodiscard]] bool empty() const noexcept {
        return cells.empty() && canonicalFields.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j;
        if (!canonicalFields.empty()) {
            for (const auto& [k, v] : canonicalFields) {
                j[k] = v;
            }
        }
        if (!cells.empty()) {
            nlohmann::json cellsJson = nlohmann::json::array();
            for (const auto& cell : cells) {
                cellsJson.push_back(cell.toJson(includeSource));
            }
            j["cells"] = std::move(cellsJson);
        }

        if (isHeader) {
            j["is_header"] = true;
        }
        j["confidence"] = confidence;

        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }

        return j;
    }
};

} // namespace edmo::ocr::representation
