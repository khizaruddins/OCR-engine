#pragma once

#include <ocr/OcrResult.hpp>
#include <ocr/Result.hpp>
#include <ocr/ingestion/IngestionConfig.hpp>

#include <filesystem>

namespace ocr::ingestion {

/**
 * @brief High-performance native PDF text extractor producing fully compatible OcrPageResult structures in milliseconds.
 */
class NativeTextExtractor {
public:
    explicit NativeTextExtractor(IngestionConfig config = {});
    ~NativeTextExtractor() = default;

    /**
     * @brief Extract native text and layout elements for a specific page of a PDF.
     */
    [[nodiscard]] Result<OcrPageResult> extractPage(
        const std::filesystem::path& pdfPath,
        int pageNumber,
        int targetDpi = 300
    ) const;

    [[nodiscard]] const IngestionConfig& config() const noexcept { return config_; }
    void setConfig(IngestionConfig config) noexcept { config_ = config; }

private:
    IngestionConfig config_;
};

} // namespace ocr::ingestion
