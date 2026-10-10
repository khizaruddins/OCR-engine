#pragma once

#include <ocr/OcrResult.hpp>
#include <ocr/representation/ConfidenceSummary.hpp>
#include <ocr/representation/DocumentRepresentation.hpp>
#include <ocr/representation/RepresentationBlock.hpp>
#include <ocr/representation/SemanticSection.hpp>
#include <ocr/representation/SemanticTable.hpp>

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ocr::representation {

/**
 * @brief Represents a single page at the representation layer.
 */
struct SemanticPage {
    int pageNumber = 1;
    int width = 0;
    int height = 0;
    int rotation = 0;

    std::string sourceType = "OCR";
    std::string processingStrategy = "OCR";
    std::string pageClassification = "SCANNED_IMAGE";
    ingestion::PageTiming timing;
    ingestion::PageComplexity complexity;

    std::vector<RepresentationBlock> blocks;
    std::vector<SpatialRegion> regions;
    ConfidenceSummary confidence;

    [[nodiscard]] nlohmann::json toJson(bool includeSource = true) const {
        nlohmann::json j{
            {"page_number", pageNumber},
            {"width", width},
            {"height", height},
            {"rotation", rotation},
            {"processing", {
                {"source_type", sourceType},
                {"processing_strategy", processingStrategy},
                {"page_classification", pageClassification},
                {"timing", timing.toJson()},
                {"complexity", complexity.toJson()}
            }},
            {"confidence", confidence.toJson()}
        };

        nlohmann::json blocksJson = nlohmann::json::array();
        for (const auto& b : blocks) {
            blocksJson.push_back(b.toJson(includeSource));
        }
        j["blocks"] = std::move(blocksJson);

        if (!regions.empty()) {
            nlohmann::json regionsJson = nlohmann::json::array();
            for (const auto& r : regions) {
                regionsJson.push_back(r.toJson());
            }
            j["regions"] = std::move(regionsJson);
        }

        return j;
    }
};

/**
 * @brief Top-level derived semantic document representation adhering to EDMO schema v0.1.
 */
struct SemanticDocument {
    std::string schemaVersion = "edr-0.2";
    std::string documentType = "generic"; // transcript, marksheet, resume, generic, etc.
    float typeConfidence = 1.0f;

    DocumentMetadata metadata;
    EngineInfo engine;

    ingestion::PreflightSummary preflight;
    std::string documentClassification = "UNKNOWN";
    std::string strategy = "ADAPTIVE";

    std::vector<SemanticPage> pages;
    std::vector<SemanticSection> sections;
    std::vector<SemanticTable> tables;

    // High-level normalized domain entities
    std::map<std::string, std::string> identity;    // student_name, student_id, dob, ssn_last4, etc.
    std::map<std::string, std::string> institution; // name, campus, address, registrar, etc.
    std::map<std::string, std::string> academic;    // degree, major, gpa, cum_gpa, credits_earned, graduation_date, etc.

    // Deduplicated document-level header / footer text
    std::string documentHeader;
    std::string documentFooter;

    std::vector<std::string> unclassifiedContent;
    std::vector<LowConfidenceWarning> warnings;

    ConfidenceSummary confidence;
    CompressionMetrics compression;
};

} // namespace ocr::representation
