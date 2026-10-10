#include "Binarization.hpp"
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"

#include <opencv2/imgproc.hpp>

namespace edmo::ocr {

cv::Mat Binarization::applyBinarization(
    const cv::Mat& src,
    BinarizationMethod method,
    double globalThreshold,
    int adaptiveBlockSize,
    double adaptiveC
) {
    if (src.empty()) return src;

    cv::Mat gray;
    if (src.channels() == 1) {
        gray = src;
    } else {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat dst;
    int bSize = (adaptiveBlockSize < 3) ? 3 : (adaptiveBlockSize % 2 == 0 ? adaptiveBlockSize + 1 : adaptiveBlockSize);

    switch (method) {
        case BinarizationMethod::GlobalThreshold:
            cv::threshold(gray, dst, globalThreshold, 255, cv::THRESH_BINARY);
            break;
        case BinarizationMethod::AdaptiveMean:
            cv::adaptiveThreshold(gray, dst, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, bSize, adaptiveC);
            break;
        case BinarizationMethod::AdaptiveGaussian:
            cv::adaptiveThreshold(gray, dst, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, bSize, adaptiveC);
            break;
        case BinarizationMethod::Sauvola: {
            cv::Mat grayFloat;
            gray.convertTo(grayFloat, CV_32F);
            cv::Mat mean, meanSq;
            cv::boxFilter(grayFloat, mean, CV_32F, cv::Size(bSize, bSize));
            cv::boxFilter(grayFloat.mul(grayFloat), meanSq, CV_32F, cv::Size(bSize, bSize));
            cv::Mat variance = meanSq - mean.mul(mean);
            cv::max(variance, 0.0f, variance);
            cv::Mat stdDev;
            cv::sqrt(variance, stdDev);
            double k = 0.2;
            double R = 128.0;
            cv::Mat threshold = mean.mul(1.0 + k * (stdDev / R - 1.0));
            cv::Mat cmpResult;
            cv::compare(grayFloat, threshold, cmpResult, cv::CMP_GT);
            cmpResult.convertTo(dst, CV_8U);
            break;
        }
        case BinarizationMethod::BlackHat: {
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(bSize, bSize));
            cv::Mat blackhat;
            cv::morphologyEx(gray, blackhat, cv::MORPH_BLACKHAT, kernel);
            cv::Mat normHat;
            cv::normalize(blackhat, normHat, 0, 255, cv::NORM_MINMAX);
            cv::threshold(normHat, dst, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
            break;
        }
        case BinarizationMethod::Otsu:
        default:
            cv::threshold(gray, dst, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
            break;
    }

    return dst;
}

Result<Image> Binarization::process(
    const Image& input,
    const PreprocessingConfig& config
) {
    if (input.empty()) {
        return Result<Image>::failure(ErrorCode::PreprocessingFailed, "Input image is empty for binarization");
    }

    if (!config.binarization) {
        return Result<Image>::success(input.clone());
    }

    cv::Mat binarized = applyBinarization(
        input.mat(),
        config.binarizationMethod,
        config.globalThreshold,
        config.adaptiveBlockSize,
        config.adaptiveC
    );

    return Result<Image>::success(Image(std::move(binarized)));
}

} // namespace edmo::ocr
