#pragma once

#include <edmo/ocr/Result.hpp>
#include <edmo/ocr/ingestion/IngestionConfig.hpp>
#include <edmo/ocr/ingestion/IngestionTypes.hpp>

#include <filesystem>
#include <memory>

namespace edmo::ocr::ingestion {

/**
 * @brief Fast, non-OCR PDF and image inspector. Performs validation and extracts metadata & text/image metrics.
 */
class PdfInspector {
public:
    explicit PdfInspector(IngestionConfig config = {});
    ~PdfInspector() = default;

    /**
     * @brief Inspect a document file (PDF or Image) and extract preflight metrics.
     */
    [[nodiscard]] Result<PreflightSummary> inspect(const std::filesystem::path& documentPath) const;

    /**
     * @brief Validate file existence, readability, header, and resource limits.
     */
    [[nodiscard]] Result<void> validate(const std::filesystem::path& documentPath) const;

    [[nodiscard]] const IngestionConfig& config() const noexcept { return config_; }
    void setConfig(IngestionConfig config) noexcept { config_ = config; }

private:
    IngestionConfig config_;

    [[nodiscard]] Result<PreflightSummary> inspectPdf(const std::filesystem::path& pdfPath) const;
    [[nodiscard]] Result<PreflightSummary> inspectImage(const std::filesystem::path& imagePath) const;
};

} // namespace edmo::ocr::ingestion
