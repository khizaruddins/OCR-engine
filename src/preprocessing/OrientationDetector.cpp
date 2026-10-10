#include "OrientationDetector.hpp"
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"
#include "../utils/ImageUtils.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <tesseract/baseapi.h>

namespace edmo::ocr {

static std::string resolveTessdataDir(const std::string& customDir) {
    if (!customDir.empty() && std::filesystem::exists(customDir)) {
        return customDir;
    }

    // Check project models directory
    static const std::vector<std::string> projectPaths = {
        "models/tessdata",
        "models/tessdata_best",
        "../models/tessdata",
        "../models/tessdata_best"
    };
    for (const auto& p : projectPaths) {
        if (std::filesystem::exists(p) && std::filesystem::exists(std::filesystem::path(p) / "eng.traineddata")) {
            return std::filesystem::absolute(p).string();
        }
    }

    const char* env = std::getenv("TESSDATA_PREFIX");
    if (env && std::filesystem::exists(env)) {
        if (std::filesystem::is_directory(env)) {
            return env;
        }
        auto p = std::filesystem::path(env);
        if (p.has_parent_path() && std::filesystem::exists(p.parent_path())) {
            return p.parent_path().string();
        }
    }

    // Standard Linux fallback locations
    static const std::vector<std::string> standardPaths = {
        "/usr/share/tesseract-ocr/4.00/tessdata",
        "/usr/share/tesseract-ocr/5/tessdata",
        "/usr/share/tessdata",
        "/usr/local/share/tessdata"
    };

    for (const auto& p : standardPaths) {
        if (std::filesystem::exists(p)) {
            return p;
        }
    }

    return "";
}

OrientationDetector::OrientationDetector()
    : osdApi_(std::make_unique<tesseract::TessBaseAPI>()) {}

OrientationDetector::~OrientationDetector() {
    if (osdApi_) {
        osdApi_->End();
    }
}

cv::Mat OrientationDetector::applyRotation(const cv::Mat& src, int rotationAngleDegrees) {
    if (src.empty() || rotationAngleDegrees == 0) {
        return src.clone();
    }

    cv::Mat dst;
    int normAngle = (rotationAngleDegrees % 360 + 360) % 360;

    switch (normAngle) {
        case 90:
            cv::rotate(src, dst, cv::ROTATE_90_CLOCKWISE);
            break;
        case 180:
            cv::rotate(src, dst, cv::ROTATE_180);
            break;
        case 270:
            cv::rotate(src, dst, cv::ROTATE_90_COUNTERCLOCKWISE);
            break;
        default:
            dst = src.clone();
            break;
    }

    return dst;
}

OrientationResult OrientationDetector::detectGeometricOrientation(const cv::Mat& grayMat) {
    OrientationResult res;
    if (grayMat.empty()) return res;

    // Sobel gradients to detect text line edge orientations
    cv::Mat gradX, gradY;
    cv::Sobel(grayMat, gradX, CV_32F, 1, 0, 3);
    cv::Sobel(grayMat, gradY, CV_32F, 0, 1, 3);

    cv::Mat absGradX = cv::abs(gradX);
    cv::Mat absGradY = cv::abs(gradY);

    double sumX = cv::sum(absGradX)[0];
    double sumY = cv::sum(absGradY)[0];

    // Standard horizontal text has stronger vertical gradients (Y edges between lines)
    // Sideways 90/270 text has stronger horizontal gradients (X edges between lines)
    double ratio = (sumX + 1e-5) / (sumY + 1e-5);
    if (ratio > 1.35) {
        res.angleDegrees = 90; // Indicates text lines are running vertically
        res.confidence = std::clamp(static_cast<float>((ratio - 1.0) / 2.0), 0.5f, 0.90f);
        res.source = "GEOMETRIC_GRADIENT";
        res.status = "CONFIDENT";
    } else {
        res.angleDegrees = 0;
        res.confidence = 0.80f;
        res.source = "GEOMETRIC_GRADIENT";
        res.status = "CONFIDENT";
    }

    return res;
}

Result<OrientationResult> OrientationDetector::detect(
    const Image& image,
    const std::string& tessdataDir
) {
    if (image.empty()) {
        return Result<OrientationResult>::failure(
            ErrorCode::OrientationDetectionFailed,
            "Image is empty"
        );
    }

    std::string dataPath = resolveTessdataDir(tessdataDir);
    const char* pathPtr = dataPath.empty() ? nullptr : dataPath.c_str();

    if (!osdApi_) {
        osdApi_ = std::make_unique<tesseract::TessBaseAPI>();
    }

    if (currentTessdataDir_ != dataPath) {
        int initStatus = osdApi_->Init(pathPtr, "osd", tesseract::OEM_DEFAULT);
        if (initStatus != 0) {
            initStatus = osdApi_->Init(pathPtr, "eng", tesseract::OEM_DEFAULT);
        }

        if (initStatus != 0) {
            EDMO_LOG_DEBUG("Tesseract initialization for orientation detection skipped (tessdata not found)");
            return Result<OrientationResult>::success(OrientationResult{0, 0.0f, "FALLBACK", "AMBIGUOUS"});
        }

        currentTessdataDir_ = dataPath;
        osdApi_->SetPageSegMode(tesseract::PSM_AUTO_OSD);
        osdApi_->SetVariable("debug_file", "/dev/null");
    }

    // Prepare image for orientation detection: scale to max dimension 1600px to maintain character readability
    cv::Mat analysisMat = image.mat();
    if (analysisMat.channels() == 3) {
        cv::cvtColor(analysisMat, analysisMat, cv::COLOR_BGR2GRAY);
    } else if (analysisMat.channels() == 4) {
        cv::cvtColor(analysisMat, analysisMat, cv::COLOR_BGRA2GRAY);
    }

    int maxDim = std::max(analysisMat.cols, analysisMat.rows);
    if (maxDim > 1600) {
        double scale = 1600.0 / static_cast<double>(maxDim);
        cv::resize(analysisMat, analysisMat, cv::Size(), scale, scale, cv::INTER_AREA);
    }

    Pix* pix = ImageUtils::matToPix(analysisMat);
    if (!pix) {
        return Result<OrientationResult>::success(OrientationResult{0, 0.0f, "FALLBACK", "AMBIGUOUS"});
    }

    osdApi_->SetImage(pix);
    osdApi_->SetSourceResolution(200);

    int orient_deg = 0;
    float orient_conf = 0.0f;
    const char* script_name = nullptr;
    float script_conf = 0.0f;

    bool osdSuccess = osdApi_->DetectOrientationScript(&orient_deg, &orient_conf, &script_name, &script_conf);

    pixDestroy(&pix);
    osdApi_->Clear();

    OrientationResult result;
    result.angleDegrees = (orient_deg % 360 + 360) % 360;

    if (osdSuccess && orient_conf >= 1.0f) {
        result.confidence = std::clamp(orient_conf / 10.0f, 0.60f, 0.99f);
        result.source = "TESSERACT_OSD";
        result.status = "CONFIDENT";
    } else {
        // Fallback: evaluate geometric gradient analysis
        auto geomRes = detectGeometricOrientation(analysisMat);
        if (geomRes.angleDegrees != 0 && geomRes.confidence >= 0.70f) {
            result.angleDegrees = geomRes.angleDegrees;
            result.confidence = geomRes.confidence;
            result.source = "GEOMETRIC_FALLBACK";
            result.status = "CONFIDENT";
        } else if (documentPriorAngle_.has_value() && documentPriorConfidence_ >= 0.80f) {
            result.angleDegrees = documentPriorAngle_.value();
            result.confidence = documentPriorConfidence_ * 0.90f;
            result.source = "DOCUMENT_PRIOR";
            result.status = "CONFIDENT";
        } else {
            result.angleDegrees = 0;
            result.confidence = 0.50f;
            result.source = "DEFAULT";
            result.status = "AMBIGUOUS";
        }
    }

    EDMO_LOG_DEBUG("Detected page orientation: {}° (conf: {:.2f}, src: {})",
        result.angleDegrees, result.confidence, result.source);

    return Result<OrientationResult>::success(result);
}

} // namespace edmo::ocr
