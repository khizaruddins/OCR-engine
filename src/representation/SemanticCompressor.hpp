#pragma once

#include <edmo/ocr/OcrResult.hpp>
#include <edmo/ocr/representation/DocumentRepresentation.hpp>
#include <edmo/ocr/representation/SemanticDocument.hpp>

#include <string>
#include <nlohmann/json.hpp>

namespace edmo::ocr::representation {

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

} // namespace edmo::ocr::representation
