#pragma once

#include <ocr/IOrientationDetector.hpp>
#include <ocr/Image.hpp>
#include <ocr/Result.hpp>
#include <memory>
#include <optional>

namespace tesseract {
class TessBaseAPI;
}

namespace ocr {

class OrientationDetector : public IOrientationDetector {
public:
    OrientationDetector();
    ~OrientationDetector() override;

    Result<OrientationResult> detect(
        const Image& image,
        const std::string& tessdataDir = ""
    ) override;

    static cv::Mat applyRotation(
        const cv::Mat& src,
        int rotationAngleDegrees
    );

    /**
     * @brief Evaluates text-line energy and horizontal/vertical gradients to detect 0° vs 90° orientation.
     */
    static OrientationResult detectGeometricOrientation(const cv::Mat& grayMat);

    void setDocumentPrior(int angleDegrees, float confidence) noexcept {
        documentPriorAngle_ = angleDegrees;
        documentPriorConfidence_ = confidence;
    }

    void clearDocumentPrior() noexcept {
        documentPriorAngle_.reset();
        documentPriorConfidence_ = 0.0f;
    }

private:
    std::unique_ptr<tesseract::TessBaseAPI> osdApi_;
    std::string currentTessdataDir_;
    std::optional<int> documentPriorAngle_;
    float documentPriorConfidence_ = 0.0f;
};

} // namespace ocr
