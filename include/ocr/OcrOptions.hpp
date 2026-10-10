#pragma once

#include <string>

namespace ocr {

/**
 * @brief Request-level parameters passed into the OCR provider.
 */
struct OcrOptions {
    std::string language = "eng";
    int psm = 3; // PSM_AUTO by default
    int oem = 1; // OEM_LSTM_ONLY by default
    std::string charWhitelist;
    std::string charBlacklist;
    std::string tessdataDir;
    float confidenceThreshold = 0.0f;
};

} // namespace ocr
