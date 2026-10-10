#include "Sharpen.hpp"
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"

#include <opencv2/imgproc.hpp>

namespace edmo::ocr {

cv::Mat Sharpen::applySharpen(const cv::Mat& src, double strength) {
    if (src.empty() || strength <= 0.0) return src.clone();

    cv::Mat blurred;
    cv::GaussianBlur(src, blurred, cv::Size(0, 0), 3.0);

    cv::Mat dst;
    cv::addWeighted(src, 1.0 + strength, blurred, -strength, 0, dst);
    return dst;
}

Result<Image> Sharpen::process(
    const Image& input,
    const PreprocessingConfig& config
) {
    if (input.empty()) {
        return Result<Image>::failure(ErrorCode::PreprocessingFailed, "Input image is empty for sharpening");
    }

    if (!config.sharpen) {
        return Result<Image>::success(input.clone());
    }

    cv::Mat sharpened = applySharpen(input.mat(), config.sharpenStrength);
    return Result<Image>::success(Image(std::move(sharpened)));
}

} // namespace edmo::ocr
