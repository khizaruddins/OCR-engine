#pragma once

#include <ocr/OcrConfig.hpp>
#include <ocr/Page.hpp>
#include <ocr/Result.hpp>

#include <filesystem>
#include <vector>

namespace ocr {

/**
 * @brief Abstract interface for rendering PDF pages into high-resolution images.
 */
class IPdfRenderer {
public:
    virtual ~IPdfRenderer() = default;

    /**
     * @brief Render all pages of a PDF document at specified DPI.
     */
    virtual Result<std::vector<Page>> render(
        const std::filesystem::path& pdfPath,
        const PdfConfig& config
    ) = 0;

    /**
     * @brief Query the total page count of a PDF without full rendering.
     */
    virtual Result<int> getPageCount(
        const std::filesystem::path& pdfPath
    ) = 0;
};

} // namespace ocr
