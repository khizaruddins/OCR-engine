#pragma once

#include <edmo/ocr/ingestion/IngestionConfig.hpp>
#include <edmo/ocr/ingestion/IngestionTypes.hpp>

#include <vector>

namespace edmo::ocr::ingestion {

/**
 * @brief Deterministic page classifier and document-level type aggregator.
 */
class PageClassifier {
public:
    explicit PageClassifier(IngestionConfig config = {});
    ~PageClassifier() = default;

    /**
     * @brief Classify a single page based on its text and image inspection metrics.
     */
    [[nodiscard]] PageInputType classifyPage(const PageInspection& page) const noexcept;

    /**
     * @brief Derive document-level classification from all page classifications.
     */
    [[nodiscard]] DocumentLevelType classifyDocument(const std::vector<PageInspection>& pages) const noexcept;

    [[nodiscard]] const IngestionConfig& config() const noexcept { return config_; }
    void setConfig(IngestionConfig config) noexcept { config_ = config; }

private:
    IngestionConfig config_;
};

} // namespace edmo::ocr::ingestion
