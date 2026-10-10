#pragma once

#include <edmo/ocr/Image.hpp>
#include <edmo/ocr/OcrOptions.hpp>
#include <edmo/ocr/OcrResult.hpp>
#include <edmo/ocr/Result.hpp>

#include <memory>
#include <string>

namespace edmo::ocr {

struct OcrConfig;

/**
 * @brief Abstract interface representing an OCR model provider.
 * Allows decoupling orchestration from specific backends (Tesseract, ONNX, PaddleOCR, Custom EDMO model).
 */
class IOcrProvider {
public:
    virtual ~IOcrProvider() = default;

    /**
     * @brief Process a single image page and extract text, hierarchy, bounding boxes, and confidence.
     */
    virtual Result<OcrPageResult> process(
        const Image& image,
        const OcrOptions& options,
        int pageNumber = 1
    ) = 0;

    /**
     * @brief Process an isolated cell image crop with targeted PSM and optional whitelist.
     */
    virtual Result<std::string> processCell(
        const Image& cellImage,
        int psm = 7,
        const std::string& charWhitelist = "",
        float* outConfidence = nullptr
    ) {
        (void)cellImage; (void)psm; (void)charWhitelist; (void)outConfidence;
        return Result<std::string>::failure(ErrorCode::OcrProcessingFailed, "Cell OCR not implemented for this provider");
    }

    /**
     * @brief Human-readable name of the provider backend.
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief Version of the provider backend.
     */
    [[nodiscard]] virtual std::string version() const = 0;

    /**
     * @brief Indicates whether this provider instance is safe for concurrent multi-threaded execution.
     */
    [[nodiscard]] virtual bool isThreadSafe() const = 0;
};

/**
 * @brief Factory function creating an OCR provider based on configuration.
 */
std::unique_ptr<IOcrProvider> createOcrProvider(const OcrConfig& config);

} // namespace edmo::ocr
