#pragma once

#include <ocr/Image.hpp>
#include <ocr/OcrConfig.hpp>
#include <ocr/Result.hpp>

namespace ocr {

class Denoise {
public:
    static Result<Image> process(
        const Image& input,
        const PreprocessingConfig& config
    );

    static cv::Mat applyDenoise(
        const cv::Mat& src,
        DenoiseMethod method,
        int kernelSize = 3,
        double sigma = 1.0
    );
};

} // namespace ocr
