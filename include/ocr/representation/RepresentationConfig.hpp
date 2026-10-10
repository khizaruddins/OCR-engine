#pragma once

#include <edmo/ocr/Result.hpp>

#include <filesystem>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::ocr::representation {

struct ReadingOrderConfig {
    bool enabled = true;
    bool multiColumn = true;
    double columnGutterThresholdRatio = 0.03; // Minimum whitespace width relative to page width for a gutter
};

struct SectionConfig {
    bool enabled = true;
    bool extractFields = true;
    double headingConfidenceThreshold = 0.60;
};

struct TableConfig {
    bool enabled = true;
    bool borderless = true;
    bool mergeMultiPage = true;
    double minColumnGapRatio = 0.015;
    double rowYProximityRatio = 0.60; // Max vertical difference relative to token height
    std::vector<std::string> gradeVocabulary = {
        "A", "A-", "A+", "B", "B+", "B-", "C", "C+", "C-", "D", "D+", "D-",
        "F", "P", "NP", "W", "I", "S", "U", "CR", "NC", "AU", "IP", "NR"
    };
};

struct NormalizationConfig {
    bool enabled = true;
    bool trimWhitespace = true;
    bool collapseWhitespace = true;
    bool normalizeLineBreaks = true;
};

struct NoiseReductionConfig {
    bool enabled = true;
    bool removeIsolatedSymbols = true;
    bool removeEmptyBlocks = true;
    bool preservePunctuation = true;
};

struct CompressionConfig {
    bool enabled = true;
    double lowConfidenceThreshold = 0.70;
};

struct SourceReferenceConfig {
    bool fullEdr = true;
    bool llmEdr = false; // By default omitted in LLM EDR for token efficiency
};

/**
 * @brief Top-level configuration for DocumentRepresentationEngine.
 */
struct RepresentationConfig {
    bool enabled = true;
    std::string documentType = "auto"; // "auto", "transcript", "generic", "resume"

    ReadingOrderConfig readingOrder;
    SectionConfig sections;
    TableConfig tables;
    NormalizationConfig normalization;
    NoiseReductionConfig noiseReduction;
    CompressionConfig compression;
    SourceReferenceConfig sourceReferences;

    static RepresentationConfig defaultTranscriptProfile() {
        RepresentationConfig c;
        c.documentType = "transcript";
        return c;
    }

    static Result<RepresentationConfig> fromJson(const nlohmann::json& j) {
        RepresentationConfig c;
        if (j.contains("representation")) {
            const auto& r = j["representation"];
            c.enabled = r.value("enabled", true);
            c.documentType = r.value("document_type", "auto");

            if (r.contains("reading_order")) {
                c.readingOrder.enabled = r["reading_order"].value("enabled", true);
                c.readingOrder.multiColumn = r["reading_order"].value("multi_column", true);
            }
            if (r.contains("sections")) {
                c.sections.enabled = r["sections"].value("enabled", true);
                c.sections.extractFields = r["sections"].value("extract_fields", true);
            }
            if (r.contains("tables")) {
                c.tables.enabled = r["tables"].value("enabled", true);
                c.tables.borderless = r["tables"].value("borderless", true);
                c.tables.mergeMultiPage = r["tables"].value("merge_multi_page", true);
            }
            if (r.contains("normalization")) {
                c.normalization.enabled = r["normalization"].value("enabled", true);
                c.normalization.trimWhitespace = r["normalization"].value("trim_whitespace", true);
                c.normalization.collapseWhitespace = r["normalization"].value("collapse_whitespace", true);
            }
            if (r.contains("noise_reduction")) {
                c.noiseReduction.enabled = r["noise_reduction"].value("enabled", true);
                c.noiseReduction.removeIsolatedSymbols = r["noise_reduction"].value("remove_isolated_symbols", true);
                c.noiseReduction.removeEmptyBlocks = r["noise_reduction"].value("remove_empty_blocks", true);
            }
            if (r.contains("source_references")) {
                c.sourceReferences.fullEdr = r["source_references"].value("full_edr", true);
                c.sourceReferences.llmEdr = r["source_references"].value("llm_edr", false);
            }
        }
        return Result<RepresentationConfig>::success(c);
    }

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"representation", {
                {"enabled", enabled},
                {"document_type", documentType},
                {"reading_order", {
                    {"enabled", readingOrder.enabled},
                    {"multi_column", readingOrder.multiColumn}
                }},
                {"sections", {
                    {"enabled", sections.enabled},
                    {"extract_fields", sections.extractFields}
                }},
                {"tables", {
                    {"enabled", tables.enabled},
                    {"borderless", tables.borderless},
                    {"merge_multi_page", tables.mergeMultiPage}
                }},
                {"normalization", {
                    {"enabled", normalization.enabled},
                    {"trim_whitespace", normalization.trimWhitespace},
                    {"collapse_whitespace", normalization.collapseWhitespace}
                }},
                {"noise_reduction", {
                    {"enabled", noiseReduction.enabled},
                    {"remove_isolated_symbols", noiseReduction.removeIsolatedSymbols},
                    {"remove_empty_blocks", noiseReduction.removeEmptyBlocks}
                }},
                {"source_references", {
                    {"full_edr", sourceReferences.fullEdr},
                    {"llm_edr", sourceReferences.llmEdr}
                }}
            }}
        };
    }
};

} // namespace edmo::ocr::representation
