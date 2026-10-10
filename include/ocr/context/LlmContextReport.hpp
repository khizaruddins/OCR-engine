#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::ocr::context {

struct PageCoverageReport {
    int detected = 0;
    int represented = 0;
    double coveragePercentage = 100.0;
    std::vector<int> pagesMissing;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"pages_detected", detected},
            {"pages_represented", represented},
            {"coverage_percentage", coveragePercentage}
        };
        if (!pagesMissing.empty()) {
            j["pages_missing"] = pagesMissing;
        }
        return j;
    }
};

struct SectionItemFate {
    std::string sourceId;
    std::string type;
    std::string fate = "STRUCTURED"; // STRUCTURED, COMPACTED, DEDUPLICATED, NOISE, METADATA, CHUNKED, UNACCOUNTED_LOSS
    bool represented = true;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"source_id", sourceId},
            {"type", type},
            {"fate", fate},
            {"represented", represented}
        };
    }
};

struct SectionCoverageReport {
    size_t sectionsDetected = 0;
    size_t sectionsRepresented = 0;
    double coveragePercentage = 100.0;
    std::vector<SectionItemFate> sections;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"sections_detected", sectionsDetected},
            {"sections_represented", sectionsRepresented},
            {"coverage_percentage", coveragePercentage}
        };
        if (!sections.empty()) {
            nlohmann::json secArray = nlohmann::json::array();
            for (const auto& s : sections) {
                secArray.push_back(s.toJson());
            }
            j["sections"] = std::move(secArray);
        }
        return j;
    }
};

struct TableCoverageReport {
    size_t tablesDetected = 0;
    size_t tablesRepresented = 0;
    size_t rowsDetected = 0;
    size_t rowsRepresented = 0;
    size_t cellsDetected = 0;
    size_t cellsRepresented = 0;
    double tableCoveragePercentage = 100.0;
    double rowCoveragePercentage = 100.0;
    double cellCoveragePercentage = 100.0;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"tables_detected", tablesDetected},
            {"tables_represented", tablesRepresented},
            {"rows_detected", rowsDetected},
            {"rows_represented", rowsRepresented},
            {"cells_detected", cellsDetected},
            {"cells_represented", cellsRepresented},
            {"table_coverage_percentage", tableCoveragePercentage},
            {"row_coverage_percentage", rowCoveragePercentage},
            {"cell_coverage_percentage", cellCoveragePercentage}
        };
    }
};

struct ConfidencePreservationReport {
    size_t sourceLowConfidenceItems = 0;             ///< Unique source items < threshold
    size_t sourceItemsWithPreservedEvidence = 0;     ///< Unique source items represented in output
    double preservationPercentage = 100.0;           ///< (preserved / source) * 100, clamped to 100.0%
    size_t outputConfidenceAnnotations = 0;          ///< Total output annotations

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"source_low_confidence_items", sourceLowConfidenceItems},
            {"source_items_with_preserved_evidence", sourceItemsWithPreservedEvidence},
            {"preservation_percentage", preservationPercentage},
            {"output_confidence_annotations", outputConfidenceAnnotations}
        };
    }
};

struct UnrepresentedTextBreakdown {
    size_t deduplicated = 0;
    size_t metadata = 0;
    size_t noise = 0;
    size_t unaccounted = 0;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"deduplicated", deduplicated},
            {"metadata", metadata},
            {"noise", noise},
            {"unaccounted", unaccounted}
        };
    }
};

struct TextCoverageReport {
    std::string unit = "characters";
    size_t sourceMeaningfulText = 0;
    size_t representedMeaningfulText = 0;
    double coveragePercentage = 100.0;
    UnrepresentedTextBreakdown unrepresentedText;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"unit", unit},
            {"source_meaningful_text", sourceMeaningfulText},
            {"represented_meaningful_text", representedMeaningfulText},
            {"coverage_percentage", coveragePercentage},
            {"unrepresented_text", unrepresentedText.toJson()}
        };
    }
};

struct LossReport {
    size_t totalSourceItems = 0;
    size_t preservedItems = 0;
    size_t compactedItems = 0;
    size_t structuredItems = 0;
    size_t deduplicatedItems = 0;
    size_t noiseItems = 0;
    size_t metadataItems = 0;
    size_t chunkedItems = 0;
    size_t unaccountedLossItems = 0;

    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"total_source_items", totalSourceItems},
            {"preserved_items", preservedItems},
            {"compacted_items", compactedItems},
            {"structured_items", structuredItems},
            {"deduplicated_items", deduplicatedItems},
            {"noise_items", noiseItems},
            {"metadata_items", metadataItems},
            {"chunked_items", chunkedItems},
            {"unaccounted_loss", unaccountedLossItems}
        };
    }
};

struct CompressionReport {
    size_t rawBytes = 0;
    size_t edrFullBytes = 0;
    size_t llmContextBytes = 0;

    size_t rawCharacters = 0;
    size_t edrFullCharacters = 0;
    size_t llmContextCharacters = 0;

    size_t estimatedRawTokens = 0;
    size_t estimatedFullTokens = 0;
    size_t estimatedLlmTokens = 0;

    std::optional<double> rawToFullRatio;
    std::optional<double> fullToLlmRatio;
    std::optional<double> rawToLlmRatio;
    double meaningfulTextCoveragePercentage = 100.0;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"raw_bytes", rawBytes},
            {"edr_full_bytes", edrFullBytes},
            {"llm_context_bytes", llmContextBytes},
            {"estimated_raw_tokens", estimatedRawTokens},
            {"estimated_full_tokens", estimatedFullTokens},
            {"estimated_llm_tokens", estimatedLlmTokens},
            {"meaningful_text_coverage_percentage", meaningfulTextCoveragePercentage}
        };
        if (rawToFullRatio.has_value()) j["raw_to_full_ratio"] = *rawToFullRatio;
        else j["raw_to_full_ratio"] = nullptr;

        if (fullToLlmRatio.has_value()) j["full_to_llm_ratio"] = *fullToLlmRatio;
        else j["full_to_llm_ratio"] = nullptr;

        if (rawToLlmRatio.has_value()) j["raw_to_llm_ratio"] = *rawToLlmRatio;
        else j["raw_to_llm_ratio"] = nullptr;

        return j;
    }
};

/**
 * @brief Comprehensive audit report generated alongside llm-context.json.
 */
struct LlmContextReport {
    std::string schemaVersion = "llm-context-report-0.1";
    std::string documentId;
    std::string documentType = "transcript";
    double contextQualityScore = 100.0;  ///< Overall 0.0 - 100.0 score based on coverage & 0 loss
    std::string status = "VERIFIED_ZERO_UNACCOUNTED_LOSS";

    PageCoverageReport pageCoverage;
    SectionCoverageReport sectionCoverage;
    TableCoverageReport tableCoverage;
    ConfidencePreservationReport confidencePreservation;
    TextCoverageReport textCoverage;
    LossReport lossAnalysis;
    CompressionReport compression;

    bool isChunked = false;
    size_t chunkCount = 1;
    std::vector<std::string> warnings;
    double totalExecutionTimeMs = 0.0;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"schema_version", schemaVersion},
            {"document_id", documentId},
            {"document_type", documentType},
            {"context_quality_score", contextQualityScore},
            {"status", status},
            {"page_coverage", pageCoverage.toJson()},
            {"section_coverage", sectionCoverage.toJson()},
            {"table_coverage", tableCoverage.toJson()},
            {"confidence_preservation", confidencePreservation.toJson()},
            {"text_coverage", textCoverage.toJson()},
            {"loss_analysis", lossAnalysis.toJson()},
            {"compression", compression.toJson()},
            {"is_chunked", isChunked},
            {"chunk_count", chunkCount},
            {"execution_time_ms", totalExecutionTimeMs}
        };

        if (!warnings.empty()) {
            j["warnings"] = warnings;
        }

        return j;
    }
};

} // namespace edmo::ocr::context
