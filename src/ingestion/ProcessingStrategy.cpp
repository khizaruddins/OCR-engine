#include <edmo/ocr/ingestion/ProcessingStrategy.hpp>

namespace edmo::ocr::ingestion {

ProcessingStrategy::ProcessingStrategy(IngestionConfig config)
    : config_(config) {}

ProcessingStrategyType ProcessingStrategy::determineStrategy(
    PageInputType inputType,
    const PageInspection& page
) const noexcept {
    if (config_.forceOcr) {
        if (inputType == PageInputType::Empty) return ProcessingStrategyType::SkipEmpty;
        if (inputType == PageInputType::Corrupted) return ProcessingStrategyType::Error;
        return ProcessingStrategyType::Ocr;
    }

    if (config_.forceNativeText) {
        if (page.textLayer.characterCount > 0) return ProcessingStrategyType::NativeText;
        return ProcessingStrategyType::Ocr;
    }

    if (page.textLayer.exists && !page.textLayer.reliability.isReliable) {
        return ProcessingStrategyType::Ocr;
    }

    switch (inputType) {
        case PageInputType::DigitalText:
            if (!page.textLayer.reliability.isReliable) {
                return ProcessingStrategyType::Ocr;
            }
            return ProcessingStrategyType::NativeText;

        case PageInputType::ScannedImage:
            return ProcessingStrategyType::Ocr;

        case PageInputType::TextAndImage:
            if (page.textLayer.meaningful && page.textLayer.reliability.isReliable && page.textLayer.qualityScore >= 0.85) {
                return ProcessingStrategyType::NativeText;
            }
            return ProcessingStrategyType::Ocr;

        case PageInputType::ImageOnly:
            return ProcessingStrategyType::Ocr;

        case PageInputType::Empty:
            return ProcessingStrategyType::SkipEmpty;

        case PageInputType::Corrupted:
            return ProcessingStrategyType::Error;

        case PageInputType::Unknown:
            return ProcessingStrategyType::Ocr;
    }

    return ProcessingStrategyType::Ocr;
}

} // namespace edmo::ocr::ingestion
