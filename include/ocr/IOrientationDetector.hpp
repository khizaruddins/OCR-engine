#pragma once

#include <edmo/ocr/Image.hpp>
#include <edmo/ocr/Result.hpp>

namespace edmo::ocr {

/**
 * @brief Result of orientation detection.
 */
struct OrientationResult {
    int angleDegrees = 0; // 0, 90, 180, 270
    float confidence = 0.0f; // 0.0 - 1.0
    std::string source = "TESSERACT_OSD";
    std::string status = "CONFIDENT";
};

/**
 * @brief Interface for detecting page orientation (0, 90, 180, 270 degrees).
 */
class IOrientationDetector {
public:
    virtual ~IOrientationDetector() = default;

    /**
     * @brief Detect page orientation angle.
     */
    virtual Result<OrientationResult> detect(
        const Image& image,
        const std::string& tessdataDir = ""
    ) = 0;
};

} // namespace edmo::ocr
