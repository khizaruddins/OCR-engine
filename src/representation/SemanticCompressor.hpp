#pragma once

#include <ocr/OcrResult.hpp>
#include <ocr/representation/DocumentRepresentation.hpp>
#include <ocr/representation/SemanticDocument.hpp>

#include <string>
#include <nlohmann/json.hpp>

namespace ocr::representation {

class SemanticCompressor {
public:
    static CompressionMetrics computeMetrics(
        const OcrResult& rawResult,
        const nlohmann::json& fullEdrJson,
        const nlohmann::json& llmEdrJson
    );

    static double computeTextPreservation(
        const OcrResult& rawResult,
        const nlohmann::json& edrJson
    );
};

} // namespace ocr::representation
