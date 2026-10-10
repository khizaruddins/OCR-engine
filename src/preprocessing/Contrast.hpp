#pragma once

#include <ocr/Image.hpp>
#include <ocr/OcrConfig.hpp>
#include <ocr/Result.hpp>

namespace ocr {

class Contrast {
public:
    static Result<Image> process(
        const Image& input,
        const PreprocessingConfig& config
    );

    static cv::Mat applyContrast(
        const cv::Mat& src,
        ContrastMethod method,
        double claheClipLimit = 2.0,
        int claheGridSize = 8
    );
};

} // namespace ocr
