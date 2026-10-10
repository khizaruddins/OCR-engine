#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace edmo::ocr::ingestion {

/**
 * @brief Document-level classification enumeration.
 */
enum class DocumentLevelType {
    DigitalText,
    ScannedImage,
    Mixed,
    ImageOnly,
    TextAndImage,
    Empty,
    Corrupted,
    Unsupported,
    Unknown
};

[[nodiscard]] constexpr std::string_view documentLevelTypeToString(DocumentLevelType type) noexcept {
    switch (type) {
        case DocumentLevelType::DigitalText:  return "DIGITAL_TEXT";
        case DocumentLevelType::ScannedImage: return "SCANNED_IMAGE";
        case DocumentLevelType::Mixed:        return "MIXED";
        case DocumentLevelType::ImageOnly:    return "IMAGE_ONLY";
        case DocumentLevelType::TextAndImage: return "TEXT_AND_IMAGE";
        case DocumentLevelType::Empty:        return "EMPTY";
        case DocumentLevelType::Corrupted:    return "CORRUPTED";
        case DocumentLevelType::Unsupported:  return "UNSUPPORTED";
        case DocumentLevelType::Unknown:      return "UNKNOWN";
    }
    return "UNKNOWN";
}

[[nodiscard]] inline DocumentLevelType stringToDocumentLevelType(std::string_view str) noexcept {
    if (str == "DIGITAL_TEXT")   return DocumentLevelType::DigitalText;
    if (str == "SCANNED_IMAGE")  return DocumentLevelType::ScannedImage;
    if (str == "MIXED")          return DocumentLevelType::Mixed;
    if (str == "IMAGE_ONLY")     return DocumentLevelType::ImageOnly;
    if (str == "TEXT_AND_IMAGE") return DocumentLevelType::TextAndImage;
    if (str == "EMPTY")          return DocumentLevelType::Empty;
    if (str == "CORRUPTED")      return DocumentLevelType::Corrupted;
    if (str == "UNSUPPORTED")    return DocumentLevelType::Unsupported;
    return DocumentLevelType::Unknown;
}

/**
 * @brief Page input type enumeration.
 */
enum class PageInputType {
    DigitalText,
    ScannedImage,
    TextAndImage,
    ImageOnly,
    Empty,
    Corrupted,
    Unknown
};

[[nodiscard]] constexpr std::string_view pageInputTypeToString(PageInputType type) noexcept {
    switch (type) {
        case PageInputType::DigitalText:  return "DIGITAL_TEXT";
        case PageInputType::ScannedImage: return "SCANNED_IMAGE";
        case PageInputType::TextAndImage: return "TEXT_AND_IMAGE";
        case PageInputType::ImageOnly:    return "IMAGE_ONLY";
        case PageInputType::Empty:        return "EMPTY";
        case PageInputType::Corrupted:    return "CORRUPTED";
        case PageInputType::Unknown:      return "UNKNOWN";
    }
    return "UNKNOWN";
}

[[nodiscard]] inline PageInputType stringToPageInputType(std::string_view str) noexcept {
    if (str == "DIGITAL_TEXT")   return PageInputType::DigitalText;
    if (str == "SCANNED_IMAGE")  return PageInputType::ScannedImage;
    if (str == "TEXT_AND_IMAGE") return PageInputType::TextAndImage;
    if (str == "IMAGE_ONLY")     return PageInputType::ImageOnly;
    if (str == "EMPTY")          return PageInputType::Empty;
    if (str == "CORRUPTED")      return PageInputType::Corrupted;
    return PageInputType::Unknown;
}

/**
 * @brief Processing strategy assigned to each page.
 */
enum class ProcessingStrategyType {
    NativeText,
    Ocr,
    OcrEnhanced,
    ImageAnalysis,
    SkipEmpty,
    Error
};

[[nodiscard]] constexpr std::string_view processingStrategyToString(ProcessingStrategyType strategy) noexcept {
    switch (strategy) {
        case ProcessingStrategyType::NativeText:    return "NATIVE_TEXT";
        case ProcessingStrategyType::Ocr:           return "OCR";
        case ProcessingStrategyType::OcrEnhanced:   return "OCR_ENHANCED";
        case ProcessingStrategyType::ImageAnalysis: return "IMAGE_ANALYSIS";
        case ProcessingStrategyType::SkipEmpty:     return "SKIP_EMPTY";
        case ProcessingStrategyType::Error:         return "ERROR";
    }
    return "ERROR";
}

[[nodiscard]] inline ProcessingStrategyType stringToProcessingStrategy(std::string_view str) noexcept {
    if (str == "NATIVE_TEXT")    return ProcessingStrategyType::NativeText;
    if (str == "OCR")            return ProcessingStrategyType::Ocr;
    if (str == "OCR_ENHANCED")   return ProcessingStrategyType::OcrEnhanced;
    if (str == "IMAGE_ANALYSIS") return ProcessingStrategyType::ImageAnalysis;
    if (str == "SKIP_EMPTY")     return ProcessingStrategyType::SkipEmpty;
    return ProcessingStrategyType::Error;
}

/**
 * @brief Layout and structural complexity classification.
 */
enum class PageComplexityLevel {
    Simple,
    Moderate,
    Complex,
    Extreme
};

[[nodiscard]] constexpr std::string_view pageComplexityToString(PageComplexityLevel level) noexcept {
    switch (level) {
        case PageComplexityLevel::Simple:   return "SIMPLE";
        case PageComplexityLevel::Moderate: return "MODERATE";
        case PageComplexityLevel::Complex:  return "COMPLEX";
        case PageComplexityLevel::Extreme:  return "EXTREME";
    }
    return "SIMPLE";
}

/**
 * @brief Reliability metrics for the native text layer (detects overlapping/ghost text collisions).
 */
struct NativeTextReliability {
    bool isReliable = true;
    int suspiciousOverlapCount = 0;
    double overlapRatio = 0.0;
    std::string triggerReason;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"is_reliable", isReliable},
            {"suspicious_overlap_count", suspiciousOverlapCount},
            {"overlap_ratio", overlapRatio},
            {"trigger_reason", triggerReason}
        };
    }
};

/**
 * @brief Detailed text-layer metrics discovered during inspection.
 */
struct TextLayerMetrics {
    bool exists = false;
    bool meaningful = false;
    int characterCount = 0;
    int wordCount = 0;
    double printableCharRatio = 0.0;
    double alphabeticRatio = 0.0;
    double whitespaceRatio = 0.0;
    double avgWordLength = 0.0;
    double textBoundingBoxArea = 0.0;
    double pageArea = 0.0;
    double textDensity = 0.0;
    double qualityScore = 0.0; // 0.0 to 1.0
    NativeTextReliability reliability;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"exists", exists},
            {"meaningful", meaningful},
            {"character_count", characterCount},
            {"word_count", wordCount},
            {"printable_ratio", printableCharRatio},
            {"alphabetic_ratio", alphabeticRatio},
            {"quality_score", qualityScore},
            {"reliability", reliability.toJson()}
        };
    }
};

/**
 * @brief Image content metrics detected on a page.
 */
struct ImageMetrics {
    int count = 0;
    double totalImageArea = 0.0;
    double largestImageArea = 0.0;
    double pageArea = 0.0;
    double coverageRatio = 0.0; // totalImageArea / pageArea
    double largestImageRatio = 0.0;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"count", count},
            {"coverage_ratio", coverageRatio},
            {"largest_image_ratio", largestImageRatio}
        };
    }
};

/**
 * @brief Timing performance breakdown for an individual page.
 */
struct PageTiming {
    double preflightMs = 0.0;
    double nativeExtractionMs = 0.0;
    double ocrMs = 0.0;
    double layoutMs = 0.0;
    double edrMs = 0.0;
    double totalMs = 0.0;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"preflight_ms", preflightMs},
            {"native_extraction_ms", nativeExtractionMs},
            {"ocr_ms", ocrMs},
            {"layout_ms", layoutMs},
            {"edr_ms", edrMs},
            {"total_ms", totalMs}
        };
    }
};

/**
 * @brief Page complexity evaluation.
 */
struct PageComplexity {
    PageComplexityLevel level = PageComplexityLevel::Simple;
    double score = 0.0; // 0.0 to 1.0

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"level", pageComplexityToString(level)},
            {"score", score}
        };
    }
};

/**
 * @brief Complete inspection profile of an individual page.
 */
struct PageInspection {
    int pageNumber = 1;
    int width = 0;
    int height = 0;
    int rotation = 0; // 0, 90, 180, 270 degrees

    PageInputType inputType = PageInputType::Unknown;
    ProcessingStrategyType strategy = ProcessingStrategyType::Error;

    TextLayerMetrics textLayer;
    ImageMetrics images;
    PageComplexity complexity;
    PageTiming timing;

    bool fallbackTriggered = false;
    std::string fallbackFrom;
    std::string fallbackTo;
    std::string fallbackReason;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"page", pageNumber},
            {"input_type", pageInputTypeToString(inputType)},
            {"text_layer", textLayer.toJson()},
            {"images", images.toJson()},
            {"processing_strategy", processingStrategyToString(strategy)},
            {"timing", timing.toJson()},
            {"complexity", complexity.toJson()}
        };

        if (fallbackTriggered) {
            j["fallback"] = {
                {"triggered", true},
                {"from", fallbackFrom},
                {"to", fallbackTo},
                {"reason", fallbackReason}
            };
        }

        return j;
    }
};

/**
 * @brief Document-level performance summary.
 */
struct DocumentPerformance {
    double totalMs = 0.0;
    int pageCount = 0;
    double averageMsPerPage = 0.0;
    int ocrPages = 0;
    int nativePages = 0;
    int skippedPages = 0;
    int failedPages = 0;
    double slowestPageMs = 0.0;
    double fastestPageMs = 0.0;
    double preflightTimeMs = 0.0;
    double nativeExtractionTimeMs = 0.0;
    double ocrTimeMs = 0.0;
    double edrTimeMs = 0.0;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"total_ms", totalMs},
            {"page_count", pageCount},
            {"average_ms_per_page", averageMsPerPage},
            {"ocr_pages", ocrPages},
            {"native_pages", nativePages},
            {"skipped_pages", skippedPages},
            {"failed_pages", failedPages},
            {"slowest_page_ms", slowestPageMs},
            {"fastest_page_ms", fastestPageMs},
            {"preflight_time_ms", preflightTimeMs},
            {"native_extraction_time_ms", nativeExtractionTimeMs},
            {"ocr_time_ms", ocrTimeMs},
            {"edr_time_ms", edrTimeMs}
        };
    }
};

/**
 * @brief Overall document preflight inspection summary.
 */
struct PreflightSummary {
    std::string schemaVersion = "edmo-preflight-0.2";
    std::string fileName;
    std::string format = "PDF";
    int pageCount = 0;
    size_t fileSizeBytes = 0;

    DocumentLevelType documentType = DocumentLevelType::Unknown;
    std::vector<PageInspection> pages;
    DocumentPerformance performance;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json pagesJson = nlohmann::json::array();
        for (const auto& p : pages) {
            pagesJson.push_back(p.toJson());
        }

        return nlohmann::json{
            {"schema_version", schemaVersion},
            {"document", {
                {"file_name", fileName},
                {"format", format},
                {"page_count", pageCount},
                {"file_size_bytes", fileSizeBytes}
            }},
            {"classification", {
                {"document_type", documentLevelTypeToString(documentType)}
            }},
            {"pages", std::move(pagesJson)},
            {"performance", performance.toJson()}
        };
    }
};

} // namespace edmo::ocr::ingestion
