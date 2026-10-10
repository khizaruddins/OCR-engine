#pragma once

#include <ocr/Image.hpp>
#include <ocr/OcrConfig.hpp>
#include <ocr/Result.hpp>

#include <filesystem>
#include <memory>

namespace ocr {

/**
 * @brief Context passed to preprocessing stages when saving debug artifacts.
 */
struct DebugContext {
    bool enabled = false;
    std::filesystem::path outputDir = "debug";
    int pageNumber = 1;
};

/**
 * @brief Results from the preprocessing stage.
 */
struct PreprocessResult {
    Image image;
    int originalWidth = 0;
    int originalHeight = 0;
    double detectedSkewDegrees = 0.0;
    int detectedRotationDegrees = 0;
    float orientationConfidence = 0.0f;
    std::string orientationSource = "NONE";
    double durationMs = 0.0;
};

/**
 * @brief Interface for document image preprocessing pipeline.
 */
class IPreprocessor {
public:
    virtual ~IPreprocessor() = default;

    /**
     * @brief Apply configured preprocessing transformations to an input image.
     */
    virtual Result<PreprocessResult> process(
        const Image& input,
        const PreprocessingConfig& config,
        const DebugContext& debugCtx = {}
    ) = 0;
};

} // namespace ocr
