#pragma once

#include <ocr/IPdfRenderer.hpp>

namespace ocr {

class PdfRenderer : public IPdfRenderer {
public:
    PdfRenderer() = default;
    ~PdfRenderer() override = default;

    Result<std::vector<Page>> render(
        const std::filesystem::path& pdfPath,
        const PdfConfig& config
    ) override;

    Result<int> getPageCount(
        const std::filesystem::path& pdfPath
    ) override;
};

} // namespace ocr
