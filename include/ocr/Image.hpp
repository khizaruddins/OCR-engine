#pragma once

#include <filesystem>
#include <memory>
#include <opencv2/core.hpp>

namespace edmo::ocr {

/**
 * @brief High-level RAII image abstraction wrapping underlying OpenCV cv::Mat.
 * Keeps OpenCV implementation details isolated from downstream consumers where appropriate.
 */
class Image {
public:
    Image() = default;
    explicit Image(cv::Mat mat) : mat_(std::move(mat)) {}
    ~Image() = default;

    Image(const Image&) = default;
    Image& operator=(const Image&) = default;
    Image(Image&&) noexcept = default;
    Image& operator=(Image&&) noexcept = default;

    [[nodiscard]] int width() const noexcept { return mat_.cols; }
    [[nodiscard]] int height() const noexcept { return mat_.rows; }
    [[nodiscard]] int channels() const noexcept { return mat_.channels(); }
    [[nodiscard]] int depth() const noexcept { return mat_.depth(); }
    [[nodiscard]] size_t totalBytes() const noexcept { return mat_.total() * mat_.elemSize(); }
    [[nodiscard]] bool empty() const noexcept { return mat_.empty(); }

    [[nodiscard]] const cv::Mat& mat() const noexcept { return mat_; }
    [[nodiscard]] cv::Mat& mat() noexcept { return mat_; }

    [[nodiscard]] Image clone() const {
        return Image(mat_.clone());
    }

    [[nodiscard]] const uint8_t* data() const noexcept { return mat_.data; }
    [[nodiscard]] uint8_t* data() noexcept { return mat_.data; }
    [[nodiscard]] size_t step() const noexcept { return mat_.step; }

    /**
     * @brief Save the image to a given filesystem destination.
     */
    bool save(const std::filesystem::path& outputPath) const;

    /**
     * @brief Load an image from a filesystem destination.
     */
    static Image fromFile(const std::filesystem::path& inputPath);

private:
    cv::Mat mat_;
};

} // namespace edmo::ocr
