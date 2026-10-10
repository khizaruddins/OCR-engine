#include <ocr/IOcrProvider.hpp>
#include <ocr/OcrConfig.hpp>
#include "TesseractOcrProvider.hpp"
#include "../logging/Logger.hpp"

namespace ocr {

std::unique_ptr<IOcrProvider> createOcrProvider(const OcrConfig& config) {
    if (config.ocr.provider.empty() || config.ocr.provider == "tesseract") {
        return std::make_unique<TesseractOcrProvider>();
    }

    EDMO_LOG_ERROR("Requested OCR provider '{}' is not supported in v0.1. Available: 'tesseract'", config.ocr.provider);
    return nullptr;
}

} // namespace ocr
