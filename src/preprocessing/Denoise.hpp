#pragma once

#include <edmo/ocr/Image.hpp>
#include <edmo/ocr/OcrConfig.hpp>
#include <edmo/ocr/Result.hpp>

namespace edmo::ocr {

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

} // namespace edmo::ocr
