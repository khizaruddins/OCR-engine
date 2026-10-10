#pragma once

#include <cstddef>
#include <nlohmann/json.hpp>

namespace ocr::ingestion {

/**
 * @brief Configuration parameters and thresholds for document ingestion and preflight.
 */
struct IngestionConfig {
    // Quality & text-layer detection thresholds
    int minMeaningfulCharCount = 50;
    int minMeaningfulWordCount = 10;
    double minPrintableRatio = 0.85;
    double minQualityScore = 0.70;

    // Image presence thresholds
    double maxDigitalImageCoverage = 0.80;
    double minScannedImageCoverage = 0.50;

    // Manual overrides and execution modes
    bool forceOcr = false;
    bool forceNativeText = false;
    bool enableAdaptiveOcr = true;
    bool enablePageParallelism = false;

    // Resource limits
    size_t maxFileSizeMb = 100;
    int maxPages = 500;
    size_t maxPagePixels = 25000000;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"min_meaningful_char_count", minMeaningfulCharCount},
            {"min_meaningful_word_count", minMeaningfulWordCount},
            {"min_printable_ratio", minPrintableRatio},
            {"min_quality_score", minQualityScore},
            {"max_digital_image_coverage", maxDigitalImageCoverage},
            {"min_scanned_image_coverage", minScannedImageCoverage},
            {"force_ocr", forceOcr},
            {"force_native_text", forceNativeText},
            {"enable_adaptive_ocr", enableAdaptiveOcr},
            {"enable_page_parallelism", enablePageParallelism},
            {"limits", {
                {"max_file_size_mb", maxFileSizeMb},
                {"max_pages", maxPages},
                {"max_page_pixels", maxPagePixels}
            }}
        };
    }

    static IngestionConfig fromJson(const nlohmann::json& j) {
        IngestionConfig cfg;
        cfg.minMeaningfulCharCount = j.value("min_meaningful_char_count", 50);
        cfg.minMeaningfulWordCount = j.value("min_meaningful_word_count", 10);
        cfg.minPrintableRatio = j.value("min_printable_ratio", 0.85);
        cfg.minQualityScore = j.value("min_quality_score", 0.70);
        cfg.maxDigitalImageCoverage = j.value("max_digital_image_coverage", 0.80);
        cfg.minScannedImageCoverage = j.value("min_scanned_image_coverage", 0.50);
        cfg.forceOcr = j.value("force_ocr", false);
        cfg.forceNativeText = j.value("force_native_text", false);
        cfg.enableAdaptiveOcr = j.value("enable_adaptive_ocr", true);
        cfg.enablePageParallelism = j.value("enable_page_parallelism", false);

        if (j.contains("limits") && j["limits"].is_object()) {
            cfg.maxFileSizeMb = j["limits"].value("max_file_size_mb", size_t(100));
            cfg.maxPages = j["limits"].value("max_pages", 500);
            cfg.maxPagePixels = j["limits"].value("max_page_pixels", size_t(25000000));
        }
        return cfg;
    }
};

} // namespace ocr::ingestion
