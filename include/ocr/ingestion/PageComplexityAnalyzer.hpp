#pragma once

#include <edmo/ocr/OcrResult.hpp>
#include <edmo/ocr/ingestion/IngestionTypes.hpp>

namespace edmo::ocr::ingestion {

/**
 * @brief Lightweight layout complexity analyzer for extracted page content.
 */
class PageComplexityAnalyzer {
public:
    PageComplexityAnalyzer() = default;
    ~PageComplexityAnalyzer() = default;

    /**
     * @brief Analyze page complexity from extracted OCR or native blocks.
     */
    [[nodiscard]] PageComplexity analyze(const OcrPageResult& pageResult) const noexcept;
};

} // namespace edmo::ocr::ingestion
