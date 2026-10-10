#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::context {

/**
 * @brief Explicit disposition assigned to every item from Full EDR.
 */
enum class ItemFate {
    Preserved,          ///< Item preserved directly in context
    Compacted,          ///< Item compacted into a token-efficient string/array
    Structured,         ///< Item transformed into a structured table/section/field
    Deduplicated,       ///< Item identified as multi-page repeated banner/boilerplate
    Noise,              ///< Item identified as isolated OCR symbol or non-content artifact
    Metadata,           ///< Item transformed into document metadata (e.g. page count)
    Chunked,            ///< Item placed into a secondary semantic chunk
    Unaccounted         ///< ERROR STATE: item dropped without explicit disposition
};

inline std::string_view itemFateToString(ItemFate fate) noexcept {
    switch (fate) {
        case ItemFate::Preserved: return "PRESERVED";
        case ItemFate::Compacted: return "COMPACTED";
        case ItemFate::Structured: return "STRUCTURED";
        case ItemFate::Deduplicated: return "DEDUPLICATED";
        case ItemFate::Noise: return "NOISE";
        case ItemFate::Metadata: return "METADATA";
        case ItemFate::Chunked: return "CHUNKED";
        case ItemFate::Unaccounted: default: return "UNACCOUNTED";
    }
}

/**
 * @brief Traceable content item tracked by inventory and loss accounting.
 */
struct ContentItem {
    std::string id;
    std::string type;       ///< block, table, row, cell, header, footer, etc.
    int pageNumber = 1;
    std::string text;
    float confidence = 1.0f;
    bool isLowConfidence = false;
    ItemFate fate = ItemFate::Unaccounted;
    std::string fateReason;
};

/**
 * @brief Low-confidence cell coordinate annotation.
 */
struct CellConfidenceAnnotation {
    size_t rowIndex = 0;
    size_t colIndex = 0;
    std::string cellText;
    float confidence = 0.0f;
    std::string warning;
};

/**
 * @brief Compact, token-efficient table model for LLM context.
 */
struct LlmContextTable {
    std::string id;
    std::string title;
    std::string tableType = "generic_table";
    int pageStart = 1;
    int pageEnd = 1;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<CellConfidenceAnnotation> lowConfidenceCells;
    std::string sourceRegion;

    [[nodiscard]] nlohmann::json toJson(bool traceable = false) const {
        nlohmann::json j{
            {"table_id", id},
            {"type", tableType},
            {"page_range", {pageStart, pageEnd}},
            {"headers", headers},
            {"rows", rows}
        };

        if (!title.empty()) {
            j["title"] = title;
        }

        if (!lowConfidenceCells.empty()) {
            nlohmann::json confJson = nlohmann::json::array();
            for (const auto& c : lowConfidenceCells) {
                confJson.push_back({
                    {"row", c.rowIndex},
                    {"col", c.colIndex},
                    {"text", c.cellText},
                    {"confidence", c.confidence},
                    {"warning", c.warning}
                });
            }
            j["low_confidence_cells"] = std::move(confJson);
        }

        if (traceable && !sourceRegion.empty()) {
            j["source_region"] = sourceRegion;
        }

        return j;
    }
};

/**
 * @brief Compact section container preserving all structured and unclassified content.
 */
struct LlmContextSection {
    std::string id;
    std::string type;       ///< student_information, institution_information, academic_records, etc.
    std::string title;
    int pageStart = 1;
    int pageEnd = 1;
    std::map<std::string, std::string> fields;
    std::vector<std::string> paragraphs;
    std::vector<LlmContextTable> tables;
    std::vector<std::string> unclassified;

    [[nodiscard]] nlohmann::json toJson(bool traceable = false) const {
        nlohmann::json j{
            {"type", type}
        };

        if (!title.empty()) {
            j["title"] = title;
        }

        if (pageStart == pageEnd) {
            j["page"] = pageStart;
        } else {
            j["page_range"] = {pageStart, pageEnd};
        }

        if (!fields.empty()) {
            nlohmann::json fJson = nlohmann::json::object();
            for (const auto& [k, v] : fields) {
                fJson[k] = v;
            }
            j["fields"] = std::move(fJson);
        }

        if (!paragraphs.empty()) {
            if (paragraphs.size() == 1) {
                j["text"] = paragraphs.front();
            } else {
                j["paragraphs"] = paragraphs;
            }
        }

        if (!tables.empty()) {
            nlohmann::json tJson = nlohmann::json::array();
            for (const auto& t : tables) {
                tJson.push_back(t.toJson(traceable));
            }
            j["tables"] = std::move(tJson);
        }

        if (!unclassified.empty()) {
            j["unclassified"] = unclassified;
        }

        return j;
    }
};

/**
 * @brief Standalone chunk when token budget necessitates semantic chunking.
 */
struct LlmContextChunk {
    std::string chunkId;
    std::string chunkType;
    int pageStart = 1;
    int pageEnd = 1;
    size_t chunkIndex = 0;
    size_t totalChunks = 1;
    std::map<std::string, std::string> globalContextSummary;
    std::vector<LlmContextSection> sections;
    std::vector<LlmContextTable> tables;
    size_t estimatedTokens = 0;

    [[nodiscard]] nlohmann::json toJson(bool traceable = false) const {
        nlohmann::json j{
            {"chunk_id", chunkId},
            {"chunk_type", chunkType},
            {"chunk_index", chunkIndex},
            {"total_chunks", totalChunks},
            {"page_range", {pageStart, pageEnd}},
            {"estimated_tokens", estimatedTokens}
        };

        if (!globalContextSummary.empty()) {
            nlohmann::json gJson = nlohmann::json::object();
            for (const auto& [k, v] : globalContextSummary) {
                gJson[k] = v;
            }
            j["global_context"] = std::move(gJson);
        }

        if (!sections.empty()) {
            nlohmann::json sJson = nlohmann::json::array();
            for (const auto& s : sections) {
                sJson.push_back(s.toJson(traceable));
            }
            j["sections"] = std::move(sJson);
        }

        if (!tables.empty()) {
            nlohmann::json tJson = nlohmann::json::array();
            for (const auto& t : tables) {
                tJson.push_back(t.toJson(traceable));
            }
            j["tables"] = std::move(tJson);
        }

        return j;
    }
};

/**
 * @brief Top-level root container for the Loss-Aware LLM Context.
 */
struct LlmContext {
    std::string schemaVersion = "llm-context-0.1";
    std::string documentType = "transcript";
    int pageCount = 1;
    std::vector<int> pagesPresent;
    std::string language = "eng";

    // Deduplicated document-level repeated headers and footers
    std::string documentHeader;
    std::string documentFooter;

    // Structured profiles
    std::map<std::string, std::string> student;
    std::map<std::string, std::string> institution;
    std::map<std::string, std::string> academic;

    // Content containers
    std::vector<LlmContextSection> sections;
    std::vector<LlmContextTable> tables;
    std::vector<std::string> unclassifiedContent;
    std::vector<std::string> warnings;

    // Semantic chunking
    bool isChunked = false;
    std::vector<LlmContextChunk> chunks;

    size_t estimatedTokens = 0;

    [[nodiscard]] nlohmann::json toJson(bool traceable = false) const {
        nlohmann::json j{
            {"schema_version", schemaVersion},
            {"document", {
                {"type", documentType},
                {"page_count", pageCount},
                {"pages_present", pagesPresent},
                {"language", language}
            }}
        };

        if (!documentHeader.empty()) {
            j["document_header"] = documentHeader;
        }

        if (!documentFooter.empty()) {
            j["document_footer"] = documentFooter;
        }

        if (!student.empty()) {
            nlohmann::json stJson = nlohmann::json::object();
            for (const auto& [k, v] : student) {
                stJson[k] = v;
            }
            j["student"] = std::move(stJson);
        }

        if (!institution.empty()) {
            nlohmann::json inJson = nlohmann::json::object();
            for (const auto& [k, v] : institution) {
                inJson[k] = v;
            }
            j["institution"] = std::move(inJson);
        }

        if (!academic.empty()) {
            nlohmann::json acJson = nlohmann::json::object();
            for (const auto& [k, v] : academic) {
                acJson[k] = v;
            }
            j["academic"] = std::move(acJson);
        }

        if (!sections.empty()) {
            nlohmann::json secJson = nlohmann::json::array();
            for (const auto& s : sections) {
                secJson.push_back(s.toJson(traceable));
            }
            j["sections"] = std::move(secJson);
        }

        if (!tables.empty()) {
            nlohmann::json tblJson = nlohmann::json::array();
            for (const auto& t : tables) {
                tblJson.push_back(t.toJson(traceable));
            }
            j["tables"] = std::move(tblJson);
        }

        if (!unclassifiedContent.empty()) {
            j["unclassified_content"] = unclassifiedContent;
        }

        if (!warnings.empty()) {
            j["warnings"] = warnings;
        }

        if (isChunked && !chunks.empty()) {
            j["is_chunked"] = true;
            nlohmann::json chJson = nlohmann::json::array();
            for (const auto& c : chunks) {
                chJson.push_back(c.toJson(traceable));
            }
            j["chunks"] = std::move(chJson);
        }

        j["estimated_tokens"] = estimatedTokens;

        return j;
    }
};

} // namespace ocr::context
