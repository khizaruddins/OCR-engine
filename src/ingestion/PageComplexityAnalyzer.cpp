#include <edmo/ocr/ingestion/PageComplexityAnalyzer.hpp>

#include <algorithm>

namespace edmo::ocr::ingestion {

PageComplexity PageComplexityAnalyzer::analyze(const OcrPageResult& pageResult) const noexcept {
    int totalTokens = 0;
    int totalLines = 0;
    int totalBlocks = static_cast<int>(pageResult.blocks.size());
    double totalTokenArea = 0.0;

    for (const auto& b : pageResult.blocks) {
        totalLines += static_cast<int>(b.lines.size());
        for (const auto& l : b.lines) {
            totalTokens += static_cast<int>(l.tokens.size());
            for (const auto& t : l.tokens) {
                totalTokenArea += (t.bbox.width * t.bbox.height);
            }
        }
    }

    double pageArea = (pageResult.width > 0 && pageResult.height > 0)
        ? (pageResult.width * static_cast<double>(pageResult.height))
        : 1.0;

    double tokenScore = std::min(1.0, static_cast<double>(totalTokens) / 600.0);
    double blockScore = std::min(1.0, static_cast<double>(totalBlocks) / 25.0);
    double densityScore = std::clamp(totalTokenArea / pageArea, 0.0, 1.0);

    double score = std::clamp(
        (0.35 * tokenScore) +
        (0.35 * blockScore) +
        (0.30 * densityScore),
        0.0, 1.0
    );

    PageComplexity complexity;
    complexity.score = score;

    if (score < 0.25) {
        complexity.level = PageComplexityLevel::Simple;
    } else if (score < 0.55) {
        complexity.level = PageComplexityLevel::Moderate;
    } else if (score < 0.80) {
        complexity.level = PageComplexityLevel::Complex;
    } else {
        complexity.level = PageComplexityLevel::Extreme;
    }

    return complexity;
}

} // namespace edmo::ocr::ingestion
