#pragma once

#include <edmo/ocr/Image.hpp>
#include <edmo/ocr/OcrConfig.hpp>
#include <edmo/ocr/Result.hpp>

namespace edmo::ocr {

struct DeskewResult {
    Image image;
    double angleDegrees = 0.0;
};

class Deskew {
public:
    static Result<DeskewResult> process(
        const Image& input,
        const PreprocessingConfig& config
    );

    static double detectSkewAngle(
        const cv::Mat& grayMat,
        DeskewMethod method = DeskewMethod::HoughLines,
        double maxAngle = 10.0
    );

    static cv::Mat rotateImage(
        const cv::Mat& src,
        double angleDegrees
    );
};

} // namespace edmo::ocr
