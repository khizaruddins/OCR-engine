#pragma once

#include <edmo/ocr/Result.hpp>
#include <edmo/ocr/ingestion/IngestionConfig.hpp>
#include <edmo/ocr/ingestion/IngestionTypes.hpp>
#include <edmo/ocr/ingestion/PageClassifier.hpp>
#include <edmo/ocr/ingestion/PdfInspector.hpp>
#include <edmo/ocr/ingestion/ProcessingStrategy.hpp>

#include <filesystem>
#include <memory>

namespace edmo::ocr::ingestion {

/**
 * @brief High-level document ingestion and preflight orchestrator.
 */
class DocumentIngestor {
public:
    explicit DocumentIngestor(IngestionConfig config = {});
    ~DocumentIngestor() = default;

    /**
     * @brief Ingest and inspect document end-to-end, producing a preflight classification and strategy summary.
     */
    [[nodiscard]] Result<PreflightSummary> ingest(const std::filesystem::path& documentPath) const;

    [[nodiscard]] const IngestionConfig& config() const noexcept { return config_; }
    void setConfig(IngestionConfig config) noexcept;

private:
    IngestionConfig config_;
    PdfInspector inspector_;
    PageClassifier classifier_;
    ProcessingStrategy strategyEngine_;
};

} // namespace edmo::ocr::ingestion
