#pragma once

#include <edmo/ocr/Image.hpp>
#include <edmo/ocr/OcrConfig.hpp>
#include <edmo/ocr/Result.hpp>

namespace edmo::ocr {

class Sharpen {
public:
    static Result<Image> process(
        const Image& input,
        const PreprocessingConfig& config
    );

    static cv::Mat applySharpen(
        const cv::Mat& src,
        double strength = 1.0
    );
};

} // namespace edmo::ocr
