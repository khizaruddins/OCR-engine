#pragma once

#include <edmo/ocr/Image.hpp>
#include <edmo/ocr/OcrConfig.hpp>
#include <edmo/ocr/Result.hpp>

namespace edmo::ocr {

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

} // namespace edmo::ocr
