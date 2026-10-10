#include <edmo/ocr/ingestion/DocumentIngestor.hpp>
#include "../logging/Logger.hpp"

namespace edmo::ocr::ingestion {

DocumentIngestor::DocumentIngestor(IngestionConfig config)
    : config_(config),
      inspector_(config),
      classifier_(config),
      strategyEngine_(config) {}

void DocumentIngestor::setConfig(IngestionConfig config) noexcept {
    config_ = config;
    inspector_.setConfig(config);
    classifier_.setConfig(config);
    strategyEngine_.setConfig(config);
}

Result<PreflightSummary> DocumentIngestor::ingest(const std::filesystem::path& documentPath) const {
    EDMO_LOG_DEBUG("Starting document ingestion and preflight for: {}", documentPath.string());

    auto inspectRes = inspector_.inspect(documentPath);
    if (!inspectRes) {
        EDMO_LOG_ERROR("Document inspection failed: {}", inspectRes.error().message);
        return Result<PreflightSummary>::failure(inspectRes.error());
    }

    auto summary = std::move(inspectRes.value());

    for (auto& p : summary.pages) {
        p.inputType = classifier_.classifyPage(p);
        p.strategy = strategyEngine_.determineStrategy(p.inputType, p);

        EDMO_LOG_DEBUG("Page {} preflight: input_type={}, strategy={}, meaningful_text={}, quality={:.2f}",
            p.pageNumber,
            pageInputTypeToString(p.inputType),
            processingStrategyToString(p.strategy),
            p.textLayer.meaningful,
            p.textLayer.qualityScore);
    }

    summary.documentType = classifier_.classifyDocument(summary.pages);

    EDMO_LOG_INFO("Document ingestion complete: type={}, pages={}, format={}",
        documentLevelTypeToString(summary.documentType),
        summary.pageCount,
        summary.format);

    return Result<PreflightSummary>::success(std::move(summary));
}

} // namespace edmo::ocr::ingestion
