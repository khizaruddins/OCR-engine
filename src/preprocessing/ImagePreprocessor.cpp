#include "ImagePreprocessor.hpp"
#include <ocr/ErrorCode.hpp>
#include "Binarization.hpp"
#include "Contrast.hpp"
#include "Denoise.hpp"
#include "Sharpen.hpp"
#include "../logging/Logger.hpp"
#include "../utils/Timer.hpp"

#include <opencv2/imgproc.hpp>

namespace ocr {

ImagePreprocessor::ImagePreprocessor()
    : orientationDetector_(std::make_unique<OrientationDetector>()) {}

void ImagePreprocessor::saveDebugImage(
    const Image& img,
    const DebugContext& ctx,
    const std::string& stepName
) {
    if (!ctx.enabled || img.empty()) return;

    std::filesystem::path dir = ctx.outputDir;
    std::string filename = "page-" + std::to_string(ctx.pageNumber) + "-" + stepName + ".png";
    std::filesystem::path fullPath = dir / filename;

    img.save(fullPath);
    EDMO_LOG_DEBUG("Saved debug image: {}", fullPath.string());
}

Result<PreprocessResult> ImagePreprocessor::process(
    const Image& input,
    const PreprocessingConfig& config,
    const DebugContext& debugCtx
) {
    if (input.empty()) {
        return Result<PreprocessResult>::failure(
            ErrorCode::PreprocessingFailed,
            "Input image is empty"
        );
    }

    Timer timer;
    PreprocessResult result;
    result.originalWidth = input.width();
    result.originalHeight = input.height();
    result.image = input.clone();

    saveDebugImage(result.image, debugCtx, "01-original");

    if (!config.enabled) {
        result.durationMs = timer.elapsedMs();
        saveDebugImage(result.image, debugCtx, "final");
        return Result<PreprocessResult>::success(std::move(result));
    }

    // 1. Convert to Grayscale early to accelerate all subsequent stages
    if (config.grayscale && result.image.channels() > 1) {
        cv::Mat gray;
        if (result.image.channels() == 4) {
            cv::cvtColor(result.image.mat(), gray, cv::COLOR_BGRA2GRAY);
        } else {
            cv::cvtColor(result.image.mat(), gray, cv::COLOR_BGR2GRAY);
        }
        result.image = Image(std::move(gray));
        saveDebugImage(result.image, debugCtx, "02-grayscale");
    }

    // 2. Orientation Detection & Correction
    if (config.orientation && orientationDetector_) {
        auto orientRes = orientationDetector_->detect(result.image);
        if (orientRes) {
            result.detectedRotationDegrees = orientRes.value().angleDegrees;
            result.orientationConfidence = orientRes.value().confidence;
            result.orientationSource = orientRes.value().source;

            if (result.detectedRotationDegrees != 0) {
                cv::Mat rotated = OrientationDetector::applyRotation(
                    result.image.mat(),
                    result.detectedRotationDegrees
                );
                result.image = Image(std::move(rotated));
                EDMO_LOG_DEBUG("Applied page rotation correction: {}° (source: {}, conf: {:.2f})",
                    result.detectedRotationDegrees, result.orientationSource, result.orientationConfidence);
                saveDebugImage(result.image, debugCtx, "03-oriented");
            }
        }
    }

    // 3. Deskew
    if (config.deskew) {
        auto deskewRes = Deskew::process(result.image, config);
        if (deskewRes) {
            result.detectedSkewDegrees = deskewRes.value().angleDegrees;
            result.image = std::move(deskewRes.value().image);
            if (std::abs(result.detectedSkewDegrees) >= config.minSkewAngleThreshold) {
                saveDebugImage(result.image, debugCtx, "04-deskewed");
            }
        }
    }

    // 4. Denoise
    if (config.denoise) {
        auto denoiseRes = Denoise::process(result.image, config);
        if (denoiseRes) {
            result.image = std::move(denoiseRes.value());
            saveDebugImage(result.image, debugCtx, "05-denoised");
        }
    }

    // 5. Contrast Enhancement
    if (config.contrast) {
        auto contrastRes = Contrast::process(result.image, config);
        if (contrastRes) {
            result.image = std::move(contrastRes.value());
            saveDebugImage(result.image, debugCtx, "06-contrast");
        }
    }

    // 6. Binarization
    if (config.binarization) {
        auto binRes = Binarization::process(result.image, config);
        if (binRes) {
            result.image = std::move(binRes.value());
            saveDebugImage(result.image, debugCtx, "07-binarized");
        }
    }

    // 7. Sharpen
    if (config.sharpen) {
        auto sharpRes = Sharpen::process(result.image, config);
        if (sharpRes) {
            result.image = std::move(sharpRes.value());
            saveDebugImage(result.image, debugCtx, "08-sharpened");
        }
    }

    saveDebugImage(result.image, debugCtx, "final");
    result.durationMs = timer.elapsedMs();

    EDMO_LOG_DEBUG("Preprocessing completed in {:.2f} ms (deskew: {:.2f}°, orientation: {}°)",
        result.durationMs, result.detectedSkewDegrees, result.detectedRotationDegrees);

    return Result<PreprocessResult>::success(std::move(result));
}

} // namespace ocr
