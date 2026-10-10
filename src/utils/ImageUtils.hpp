#pragma once

#include <edmo/ocr/Image.hpp>
#include <edmo/ocr/Result.hpp>

#include <filesystem>
#include <leptonica/allheaders.h>
#include <opencv2/core.hpp>
#include <poppler-image.h>

namespace edmo::ocr {

class ImageUtils {
public:
    /**
     * @brief Convert Poppler rendered image to OpenCV cv::Mat.
     */
    static Result<cv::Mat> popplerImageToMat(const poppler::image& pImg);

    /**
     * @brief Convert OpenCV cv::Mat to Leptonica Pix structure.
     * Caller is responsible for calling pixDestroy(&pix).
     */
    static Pix* matToPix(const cv::Mat& mat);

    /**
     * @brief Convert Leptonica Pix structure to OpenCV cv::Mat.
     */
    static cv::Mat pixToMat(Pix* pix);

    /**
     * @brief Load image file safely into OpenCV Mat with dimensions limit.
     */
    static Result<cv::Mat> loadMat(
        const std::filesystem::path& path,
        int maxDimension = 20000
    );

    /**
     * @brief Save OpenCV Mat safely creating parent directories if necessary.
     */
    static bool saveMat(
        const std::filesystem::path& path,
        const cv::Mat& mat
    );
};

} // namespace edmo::ocr
