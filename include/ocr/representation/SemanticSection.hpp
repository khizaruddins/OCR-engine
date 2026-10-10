#pragma once

#include <edmo/ocr/representation/ConfidenceSummary.hpp>
#include <edmo/ocr/representation/DocumentRepresentation.hpp>
#include <edmo/ocr/representation/RepresentationBlock.hpp>
#include <edmo/ocr/representation/SemanticTable.hpp>
#include <edmo/ocr/representation/SourceReference.hpp>

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::ocr::representation {

/**
 * @brief Represents a semantic document section (e.g. Student Information, Course History).
 */
struct SemanticSection {
    std::string id;
    std::string title;
    SectionType type = SectionType::Unknown;
    float confidence = 0.0f;

    int pageStart = 1;
    int pageEnd = 1;

    std::vector<RepresentationBlock> blocks;
    std::vector<SemanticTable> tables;

    // Extracted key-value fields belonging to this section
    std::map<std::string, std::string> fields;

    ConfidenceSummary confidenceSummary;
    SourceReference source;

    [[nodiscard]] bool empty() const noexcept {
        return blocks.empty() && tables.empty() && fields.empty();
    }

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"id", id},
            {"type", sectionTypeToString(type)},
            {"title", title},
            {"page_start", pageStart},
            {"page_end", pageEnd},
            {"confidence", confidenceSummary.tokenCount > 0 ? confidenceSummary.mean : confidence}
        };

        if (!fields.empty()) {
            nlohmann::json fieldsJson = nlohmann::json::object();
            for (const auto& [k, v] : fields) {
                fieldsJson[k] = v;
            }
            j["fields"] = std::move(fieldsJson);
        }

        if (!blocks.empty()) {
            nlohmann::json blocksJson = nlohmann::json::array();
            for (const auto& block : blocks) {
                blocksJson.push_back(block.toJson(includeSource));
            }
            j["blocks"] = std::move(blocksJson);
        }

        if (!tables.empty()) {
            nlohmann::json tablesJson = nlohmann::json::array();
            for (const auto& tbl : tables) {
                tablesJson.push_back(tbl.toJson(includeSource));
            }
            j["tables"] = std::move(tablesJson);
        }

        if (includeSource && !source.empty()) {
            j["source"] = source.toJson();
        }

        return j;
    }
};

} // namespace edmo::ocr::representation
