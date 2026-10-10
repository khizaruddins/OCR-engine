#include "Contrast.hpp"
#include <ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"

#include <opencv2/imgproc.hpp>

namespace ocr {

cv::Mat Contrast::applyContrast(
    const cv::Mat& src,
    ContrastMethod method,
    double claheClipLimit,
    int claheGridSize
) {
    if (src.empty()) return src;

    cv::Mat gray;
    if (src.channels() == 1) {
        gray = src;
    } else {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat dst;
    if (method == ContrastMethod::HistogramEqualization) {
        cv::equalizeHist(gray, dst);
    } else {
        int gSize = std::max(2, claheGridSize);
        auto clahe = cv::createCLAHE(claheClipLimit, cv::Size(gSize, gSize));
        clahe->apply(gray, dst);
    }

    return dst;
}

Result<Image> Contrast::process(
    const Image& input,
    const PreprocessingConfig& config
) {
    if (input.empty()) {
        return Result<Image>::failure(ErrorCode::PreprocessingFailed, "Input image is empty for contrast enhancement");
    }

    if (!config.contrast) {
        return Result<Image>::success(input.clone());
    }

    cv::Mat enhanced = applyContrast(
        input.mat(),
        config.contrastMethod,
        config.claheClipLimit,
        config.claheGridSize
    );

    return Result<Image>::success(Image(std::move(enhanced)));
}

} // namespace ocr
