#pragma once

#include <ocr/Image.hpp>

#include <optional>

namespace ocr {

/**
 * @brief Represents a single page within a document.
 */
struct Page {
    int pageNumber = 1; // 1-indexed for user-facing and downstream consistency
    int width = 0;
    int height = 0;
    int dpi = 300;
    Image image;
    std::optional<int> detectedRotation = std::nullopt; // 0, 90, 180, 270 degrees
    double detectedSkewDegrees = 0.0;
};

} // namespace ocr
