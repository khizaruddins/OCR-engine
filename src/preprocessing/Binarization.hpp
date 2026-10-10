#pragma once

#include <ocr/Image.hpp>
#include <ocr/OcrConfig.hpp>
#include <ocr/Result.hpp>

namespace ocr {

class Binarization {
public:
    static Result<Image> process(
        const Image& input,
        const PreprocessingConfig& config
    );

    static cv::Mat applyBinarization(
        const cv::Mat& src,
        BinarizationMethod method,
        double globalThreshold = 128.0,
        int adaptiveBlockSize = 11,
        double adaptiveC = 2.0
    );
};

} // namespace ocr
