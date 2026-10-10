#pragma once

#include <ocr/Image.hpp>
#include <ocr/OcrConfig.hpp>
#include <ocr/Result.hpp>

namespace ocr {

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

} // namespace ocr
