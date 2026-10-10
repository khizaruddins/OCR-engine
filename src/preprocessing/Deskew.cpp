#include "Deskew.hpp"
#include <ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"

#include <cmath>
#include <numeric>
#include <opencv2/imgproc.hpp>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ocr {

double Deskew::detectSkewAngle(
    const cv::Mat& grayMat,
    DeskewMethod method,
    double maxAngle
) {
    if (grayMat.empty() || grayMat.rows < 10 || grayMat.cols < 10) {
        return 0.0;
    }

    // Downscale large images for fast skew detection (angle is scale-invariant)
    cv::Mat processMat = grayMat;
    int maxDim = std::max(grayMat.cols, grayMat.rows);
    if (maxDim > 1200) {
        double scale = 1200.0 / static_cast<double>(maxDim);
        cv::resize(grayMat, processMat, cv::Size(), scale, scale, cv::INTER_AREA);
    }

    if (method == DeskewMethod::MinAreaRect) {
        // Find all non-background pixel contours and calculate minAreaRect
        cv::Mat binarized;
        cv::threshold(processMat, binarized, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

        std::vector<cv::Point> pts;
        cv::findNonZero(binarized, pts);
        if (pts.size() < 100) return 0.0;

        cv::RotatedRect box = cv::minAreaRect(pts);
        double angle = box.angle;
        if (angle < -45.0) {
            angle = -(90.0 + angle);
        } else if (angle > 45.0) {
            angle = 90.0 - angle;
        } else {
            angle = -angle;
        }

        if (std::abs(angle) <= maxAngle) {
            return angle;
        }
        return 0.0;
    }

    // Default: Hough Lines with morphological line enhancement
    cv::Mat binarized;
    cv::adaptiveThreshold(
        processMat, binarized, 255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV, 15, 5
    );

    // Connect horizontal text characters into unbroken line segments
    int kernelWidth = std::max(15, processMat.cols / 60);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelWidth, 1));
    cv::Mat morph;
    cv::morphologyEx(binarized, morph, cv::MORPH_CLOSE, kernel);

    std::vector<cv::Vec4i> lines;
    int threshold = std::max(40, processMat.cols / 25);
    double minLineLength = std::max(25.0, static_cast<double>(processMat.cols) / 30.0);
    double maxLineGap = 10.0;

    cv::HoughLinesP(morph, lines, 1, M_PI / 180.0, threshold, minLineLength, maxLineGap);

    if (lines.empty()) {
        return 0.0;
    }

    std::vector<double> angles;
    angles.reserve(lines.size());

    for (const auto& l : lines) {
        double dx = static_cast<double>(l[2] - l[0]);
        double dy = static_cast<double>(l[3] - l[1]);

        if (std::abs(dx) < 1e-4) continue; // vertical line

        double angle = std::atan2(dy, dx) * 180.0 / M_PI;

        if (std::abs(angle) <= maxAngle) {
            angles.push_back(angle);
        }
    }

    if (angles.empty()) {
        return 0.0;
    }

    // Calculate median angle to ignore outliers
    std::sort(angles.begin(), angles.end());
    double medianAngle = angles[angles.size() / 2];

    return medianAngle;
}

cv::Mat Deskew::rotateImage(const cv::Mat& src, double angleDegrees) {
    if (std::abs(angleDegrees) < 1e-3 || src.empty()) {
        return src.clone();
    }

    cv::Point2f center(src.cols / 2.0f, src.rows / 2.0f);
    cv::Mat rotMat = cv::getRotationMatrix2D(center, angleDegrees, 1.0);

    // Compute bounding dimensions of the rotated image to avoid clipping
    cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), src.size(), static_cast<float>(angleDegrees)).boundingRect2f();
    rotMat.at<double>(0, 2) += bbox.width / 2.0 - center.x;
    rotMat.at<double>(1, 2) += bbox.height / 2.0 - center.y;

    cv::Mat dst;
    cv::Scalar bgScalar = (src.channels() == 1) ? cv::Scalar(255) : cv::Scalar(255, 255, 255);
    cv::warpAffine(
        src, dst, rotMat, bbox.size(),
        cv::INTER_CUBIC, cv::BORDER_CONSTANT, bgScalar
    );

    return dst;
}

Result<DeskewResult> Deskew::process(
    const Image& input,
    const PreprocessingConfig& config
) {
    if (input.empty()) {
        return Result<DeskewResult>::failure(ErrorCode::PreprocessingFailed, "Input image is empty for deskew");
    }

    cv::Mat gray;
    if (input.channels() == 3) {
        cv::cvtColor(input.mat(), gray, cv::COLOR_BGR2GRAY);
    } else if (input.channels() == 4) {
        cv::cvtColor(input.mat(), gray, cv::COLOR_BGRA2GRAY);
    } else {
        gray = input.mat();
    }

    double detectedAngle = detectSkewAngle(
        gray,
        config.deskewMethod,
        config.maxSkewAngleDegrees
    );

    DeskewResult result;
    result.angleDegrees = detectedAngle;

    if (std::abs(detectedAngle) >= config.minSkewAngleThreshold) {
        EDMO_LOG_DEBUG("Deskewing image by detected angle: {:.2f}°", detectedAngle);
        // Note: rotate with opposite sign to deskew
        cv::Mat rotated = rotateImage(input.mat(), detectedAngle);
        result.image = Image(std::move(rotated));
    } else {
        result.image = input.clone();
    }

    return Result<DeskewResult>::success(std::move(result));
}

} // namespace ocr
