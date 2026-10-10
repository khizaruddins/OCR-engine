#pragma once

#include <edmo/ocr/ingestion/IngestionConfig.hpp>
#include <edmo/ocr/ingestion/IngestionTypes.hpp>

namespace edmo::ocr::ingestion {

/**
 * @brief Strategy engine determining the cheapest correct processing path for each page.
 */
class ProcessingStrategy {
public:
    explicit ProcessingStrategy(IngestionConfig config = {});
    ~ProcessingStrategy() = default;

    /**
     * @brief Determine processing strategy for an individual page.
     */
    [[nodiscard]] ProcessingStrategyType determineStrategy(
        PageInputType inputType,
        const PageInspection& page
    ) const noexcept;

    [[nodiscard]] const IngestionConfig& config() const noexcept { return config_; }
    void setConfig(IngestionConfig config) noexcept { config_ = config; }

private:
    IngestionConfig config_;
};

} // namespace edmo::ocr::ingestion
