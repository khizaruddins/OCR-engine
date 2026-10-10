#include <ocr/ingestion/PageClassifier.hpp>

namespace ocr::ingestion {

PageClassifier::PageClassifier(IngestionConfig config)
    : config_(config) {}

PageInputType PageClassifier::classifyPage(const PageInspection& page) const noexcept {
    if (page.textLayer.exists && !page.textLayer.reliability.isReliable) {
        return PageInputType::TextAndImage;
    }

    bool isTextMeaningful = page.textLayer.meaningful ||
        (page.textLayer.exists &&
         page.textLayer.reliability.isReliable &&
         page.textLayer.characterCount >= config_.minMeaningfulCharCount &&
         page.textLayer.wordCount >= config_.minMeaningfulWordCount &&
         page.textLayer.printableCharRatio >= config_.minPrintableRatio &&
         page.textLayer.qualityScore >= config_.minQualityScore);

    if (isTextMeaningful) {
        if (page.images.coverageRatio > 0.30 && page.images.coverageRatio <= config_.maxDigitalImageCoverage) {
            return PageInputType::TextAndImage;
        }
        if (page.images.coverageRatio <= config_.maxDigitalImageCoverage) {
            return PageInputType::DigitalText;
        }
    }

    if (!page.textLayer.meaningful) {
        if (page.images.coverageRatio >= config_.minScannedImageCoverage) {
            return PageInputType::ScannedImage;
        }
        if (page.images.count > 0) {
            return PageInputType::ImageOnly;
        }
        if (page.textLayer.characterCount == 0 && page.images.count == 0) {
            return PageInputType::Empty;
        }
        return PageInputType::ScannedImage; // Default fallback for non-empty page without digital text
    }

    return PageInputType::Unknown;
}

DocumentLevelType PageClassifier::classifyDocument(const std::vector<PageInspection>& pages) const noexcept {
    if (pages.empty()) {
        return DocumentLevelType::Empty;
    }

    int digitalCount = 0;
    int scannedCount = 0;
    int imageOnlyCount = 0;
    int textAndImgCount = 0;
    int emptyCount = 0;
    int corruptedCount = 0;
    int total = static_cast<int>(pages.size());

    for (const auto& p : pages) {
        switch (p.inputType) {
            case PageInputType::DigitalText:  digitalCount++; break;
            case PageInputType::ScannedImage: scannedCount++; break;
            case PageInputType::ImageOnly:    imageOnlyCount++; break;
            case PageInputType::TextAndImage: textAndImgCount++; break;
            case PageInputType::Empty:        emptyCount++; break;
            case PageInputType::Corrupted:    corruptedCount++; break;
            case PageInputType::Unknown:      scannedCount++; break;
        }
    }

    if (corruptedCount == total) {
        return DocumentLevelType::Corrupted;
    }
    if (emptyCount == total) {
        return DocumentLevelType::Empty;
    }
    if (digitalCount == total) {
        return DocumentLevelType::DigitalText;
    }
    if ((scannedCount + imageOnlyCount) == total) {
        return DocumentLevelType::ScannedImage;
    }
    if (textAndImgCount == total) {
        return DocumentLevelType::TextAndImage;
    }
    if (digitalCount > 0 && (scannedCount > 0 || imageOnlyCount > 0)) {
        return DocumentLevelType::Mixed;
    }
    if (textAndImgCount > 0 && digitalCount > 0) {
        return DocumentLevelType::Mixed;
    }

    return DocumentLevelType::Mixed;
}

} // namespace ocr::ingestion
