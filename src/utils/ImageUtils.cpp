#include "ImageUtils.hpp"
#include <ocr/Image.hpp>
#include <ocr/ErrorCode.hpp>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

namespace ocr {

bool Image::save(const std::filesystem::path& outputPath) const {
    if (empty()) return false;
    return ImageUtils::saveMat(outputPath, mat_);
}

Image Image::fromFile(const std::filesystem::path& inputPath) {
    auto res = ImageUtils::loadMat(inputPath);
    if (!res) {
        return Image();
    }
    return Image(res.value());
}

Result<cv::Mat> ImageUtils::popplerImageToMat(const poppler::image& pImg) {
    if (!pImg.is_valid()) {
        return Result<cv::Mat>::failure(
            ErrorCode::PdfRenderingFailed,
            "Rendered Poppler image is invalid"
        );
    }

    int width = pImg.width();
    int height = pImg.height();
    int bytesPerRow = pImg.bytes_per_row();
    const char* data = pImg.const_data();

    cv::Mat resultMat;

    switch (pImg.format()) {
        case poppler::image::format_argb32: {
            cv::Mat bgra(height, width, CV_8UC4, const_cast<char*>(data), bytesPerRow);
            cv::cvtColor(bgra, resultMat, cv::COLOR_BGRA2BGR);
            break;
        }
        case poppler::image::format_rgb24: {
            cv::Mat rgb(height, width, CV_8UC3, const_cast<char*>(data), bytesPerRow);
            cv::cvtColor(rgb, resultMat, cv::COLOR_RGB2BGR);
            break;
        }
        case poppler::image::format_gray8: {
            cv::Mat gray(height, width, CV_8UC1, const_cast<char*>(data), bytesPerRow);
            resultMat = gray.clone();
            break;
        }
        case poppler::image::format_mono: {
            cv::Mat mono(height, width, CV_8UC1);
            for (int y = 0; y < height; ++y) {
                const uint8_t* srcRow = reinterpret_cast<const uint8_t*>(data + y * bytesPerRow);
                uint8_t* dstRow = mono.ptr<uint8_t>(y);
                for (int x = 0; x < width; ++x) {
                    bool bit = (srcRow[x / 8] >> (7 - (x % 8))) & 1;
                    dstRow[x] = bit ? 255 : 0;
                }
            }
            resultMat = mono;
            break;
        }
        default: {
            // Fallback: copy raw buffer
            cv::Mat fallback(height, width, CV_8UC3, const_cast<char*>(data), bytesPerRow);
            resultMat = fallback.clone();
            break;
        }
    }

    return Result<cv::Mat>::success(resultMat.clone());
}

Pix* ImageUtils::matToPix(const cv::Mat& mat) {
    if (mat.empty()) return nullptr;

    std::vector<uchar> buffer;
    if (!cv::imencode(".png", mat, buffer)) {
        return nullptr;
    }

    return pixReadMemPng(buffer.data(), buffer.size());
}

cv::Mat ImageUtils::pixToMat(Pix* pix) {
    if (!pix) return cv::Mat();

    l_uint8* data = nullptr;
    size_t size = 0;
    if (pixWriteMemPng(&data, &size, pix, 0.0f) != 0 || !data) {
        return cv::Mat();
    }

    std::vector<uchar> buffer(data, data + size);
    lept_free(data);

    return cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
}

Result<cv::Mat> ImageUtils::loadMat(
    const std::filesystem::path& path,
    int maxDimension
) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return Result<cv::Mat>::failure(ErrorCode::FileNotFound, "Image file not found", path.string());
    }

    cv::Mat img = cv::imread(path.string(), cv::IMREAD_COLOR);
    if (img.empty()) {
        return Result<cv::Mat>::failure(
            ErrorCode::ImageLoadFailed,
            "OpenCV failed to decode image (corrupted or unsupported format)",
            path.string()
        );
    }

    if (img.cols > maxDimension || img.rows > maxDimension) {
        return Result<cv::Mat>::failure(
            ErrorCode::FileTooLarge,
            "Image dimensions (" + std::to_string(img.cols) + "x" + std::to_string(img.rows) +
                ") exceed max dimension (" + std::to_string(maxDimension) + ")",
            path.string()
        );
    }

    return Result<cv::Mat>::success(img);
}

bool ImageUtils::saveMat(
    const std::filesystem::path& path,
    const cv::Mat& mat
) {
    if (mat.empty()) return false;

    std::error_code ec;
    auto parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
        std::filesystem::create_directories(parent, ec);
    }

    return cv::imwrite(path.string(), mat);
}

} // namespace ocr
