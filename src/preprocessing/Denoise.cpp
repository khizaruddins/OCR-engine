#include "Denoise.hpp"
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"

#include <opencv2/imgproc.hpp>

namespace edmo::ocr {

cv::Mat Denoise::applyDenoise(
    const cv::Mat& src,
    DenoiseMethod method,
    int kernelSize,
    double sigma
) {
    if (src.empty()) return src;

    // Ensure odd kernel size >= 3
    int k = (kernelSize < 3) ? 3 : (kernelSize % 2 == 0 ? kernelSize + 1 : kernelSize);
    cv::Mat dst;

    switch (method) {
        case DenoiseMethod::Median:
            cv::medianBlur(src, dst, k);
            break;
        case DenoiseMethod::Bilateral:
            if (src.channels() == 1) {
                cv::bilateralFilter(src, dst, k, sigma * 50.0, sigma * 50.0);
            } else {
                cv::Mat gray;
                cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
                cv::bilateralFilter(gray, dst, k, sigma * 50.0, sigma * 50.0);
            }
            break;
        case DenoiseMethod::Gaussian:
        default:
            cv::GaussianBlur(src, dst, cv::Size(k, k), sigma);
            break;
    }

    return dst;
}

Result<Image> Denoise::process(
    const Image& input,
    const PreprocessingConfig& config
) {
    if (input.empty()) {
        return Result<Image>::failure(ErrorCode::PreprocessingFailed, "Input image is empty for denoising");
    }

    if (!config.denoise) {
        return Result<Image>::success(input.clone());
    }

    cv::Mat denoised = applyDenoise(
        input.mat(),
        config.denoiseMethod,
        config.denoiseKernelSize,
        config.denoiseSigma
    );

    return Result<Image>::success(Image(std::move(denoised)));
}

} // namespace edmo::ocr
