#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace edmo::ocr {

/**
 * @brief Represents the high-level category of an input document.
 */
enum class DocumentType {
    Image,
    Pdf,
    Unknown
};

/**
 * @brief Output serialization formats supported by the engine.
 */
enum class OutputFormat {
    CompactJson,
    FullJson,
    PlainText
};

/**
 * @brief Denoising filtering algorithm to apply.
 */
enum class DenoiseMethod {
    Gaussian,
    Median,
    Bilateral
};

/**
 * @brief Contrast enhancement technique.
 */
enum class ContrastMethod {
    HistogramEqualization,
    Clahe
};

/**
 * @brief Binarization algorithm.
 */
enum class BinarizationMethod {
    GlobalThreshold,
    Otsu,
    AdaptiveMean,
    AdaptiveGaussian,
    Sauvola,
    BlackHat
};

/**
 * @brief Deskewing detection algorithm.
 */
enum class DeskewMethod {
    HoughLines,
    ProjectionProfile,
    MinAreaRect
};

/**
 * @brief Convert DocumentType to human-readable string.
 */
inline std::string_view documentTypeToString(DocumentType type) noexcept {
    switch (type) {
        case DocumentType::Image:
            return "image";
        case DocumentType::Pdf:
            return "pdf";
        case DocumentType::Unknown:
        default:
            return "unknown";
    }
}

/**
 * @brief Convert OutputFormat to string identifier.
 */
inline std::string_view outputFormatToString(OutputFormat format) noexcept {
    switch (format) {
        case OutputFormat::CompactJson:
            return "compact-json";
        case OutputFormat::FullJson:
            return "full-json";
        case OutputFormat::PlainText:
            return "text";
        default:
            return "full-json";
    }
}

} // namespace edmo::ocr
