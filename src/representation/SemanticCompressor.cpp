#include "SemanticCompressor.hpp"
#include "TextNormalizer.hpp"

#include <cctype>
#include <sstream>
#include <unordered_set>

namespace edmo::ocr::representation {

double SemanticCompressor::computeTextPreservation(
    const OcrResult& rawResult,
    const nlohmann::json& edrJson
) {
    std::string edrDump = TextNormalizer::toUpper(edrJson.dump());

    size_t totalTokens = 0;
    size_t preservedTokens = 0;

    for (const auto& page : rawResult.pages) {
        for (const auto& block : page.blocks) {
            for (const auto& line : block.lines) {
                for (const auto& token : line.tokens) {
                    std::string word = TextNormalizer::toUpper(TextNormalizer::trim(token.text));
                    if (word.empty()) continue;

                    // Filter out single noise characters
                    if (word.size() == 1 && !std::isalnum(static_cast<unsigned char>(word[0]))) {
                        continue;
                    }

                    totalTokens++;
                    if (edrDump.find(word) != std::string::npos) {
                        preservedTokens++;
                    }
                }
            }
        }
    }

    if (totalTokens == 0) return 100.0;
    return (static_cast<double>(preservedTokens) / static_cast<double>(totalTokens)) * 100.0;
}

CompressionMetrics SemanticCompressor::computeMetrics(
    const OcrResult& rawResult,
    const nlohmann::json& fullEdrJson,
    const nlohmann::json& llmEdrJson
) {
    CompressionMetrics metrics;

    std::string rawDump = rawResult.toJson(OutputFormat::FullJson).dump();
    std::string fullDump = fullEdrJson.dump();
    std::string llmDump = llmEdrJson.dump();

    metrics.rawBytes = rawDump.size();
    metrics.rawCharacters = rawDump.size();

    metrics.edrFullBytes = fullDump.size();
    metrics.edrFullCharacters = fullDump.size();

    metrics.edrLlmBytes = llmDump.size();
    metrics.edrLlmCharacters = llmDump.size();

    metrics.rawToFullRatio = metrics.edrFullBytes > 0
        ? static_cast<double>(metrics.rawBytes) / static_cast<double>(metrics.edrFullBytes)
        : 1.0;

    metrics.rawToLlmRatio = metrics.edrLlmBytes > 0
        ? static_cast<double>(metrics.rawBytes) / static_cast<double>(metrics.edrLlmBytes)
        : 1.0;

    metrics.estimatedRawTokens = metrics.rawCharacters / 4;
    metrics.estimatedFullTokens = metrics.edrFullCharacters / 4;
    metrics.estimatedLlmTokens = metrics.edrLlmCharacters / 4;

    metrics.textPreservationPercentage = computeTextPreservation(rawResult, fullEdrJson);

    return metrics;
}

} // namespace edmo::ocr::representation
