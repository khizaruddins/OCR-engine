#pragma once

#include <edmo/ocr/representation/ConfidenceSummary.hpp>
#include <edmo/ocr/representation/SemanticRow.hpp>
#include <edmo/ocr/representation/SourceReference.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::ocr::representation {

/**
 * @brief Represents a column descriptor in a SemanticTable.
 */
struct SemanticColumn {
    std::string name;
    std::string canonicalName; // mapped canonical role: course_code, course_title, credits, grade, term
    int columnIndex = 0;
    int minX = 0;
    int maxX = 0;
    float confidence = 0.0f;
    SourceReference source;

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"name", name},
            {"canonical_name", canonicalName.empty() ? name : canonicalName},
            {"column_index", columnIndex},
            {"confidence", confidence}
        };
        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }
        return j;
    }
};

/**
 * @brief Represents a reconstructed semantic table spanning one or multiple pages.
 */
struct SemanticTable {
    std::string id;
    std::string caption;
    std::string tableType = "generic_table"; // course_records, grade_summary, generic_table

    int pageStart = 1;
    int pageEnd = 1;
    bool hasHeader = false;

    std::vector<SemanticColumn> columns;
    std::vector<SemanticRow> rows;

    ConfidenceSummary confidence;
    SourceReference source;

    [[nodiscard]] bool empty() const noexcept {
        return rows.empty() && columns.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"id", id},
            {"table_type", tableType},
            {"page_start", pageStart},
            {"page_end", pageEnd},
            {"has_header", hasHeader},
            {"confidence", confidence.toJson()}
        };

        if (!caption.empty()) {
            j["caption"] = caption;
        }

        nlohmann::json colsJson = nlohmann::json::array();
        for (const auto& col : columns) {
            colsJson.push_back(col.toJson(includeSource));
        }
        j["columns"] = std::move(colsJson);

        nlohmann::json rowsJson = nlohmann::json::array();
        for (const auto& row : rows) {
            rowsJson.push_back(row.toJson(includeSource));
        }
        j["rows"] = std::move(rowsJson);

        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }

        return j;
    }
};

} // namespace edmo::ocr::representation
