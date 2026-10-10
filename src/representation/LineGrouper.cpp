#include "LineGrouper.hpp"
#include "ReadingOrder.hpp"
#include "TextNormalizer.hpp"

#include <algorithm>
#include <cmath>

namespace edmo::ocr::representation {

OcrLine LineGrouper::buildLineFromTokens(std::vector<OcrToken> tokens) {
    OcrLine line;
    if (tokens.empty()) return line;

    std::sort(tokens.begin(), tokens.end(), [](const OcrToken& a, const OcrToken& b) {
        return a.bbox.x < b.bbox.x;
    });

    BoundingBox unionBox = tokens.front().bbox;
    float sumConf = 0.0f;
    std::string text;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        unionBox = unionBox.unionWith(tok.bbox);
        sumConf += tok.confidence;

        if (i > 0) {
            // Check horizontal gap
            int gap = tok.bbox.x - tokens[i - 1].bbox.right();
            if (gap > 2) {
                text.push_back(' ');
            }
        }
        text.append(tok.text);
    }

    line.bbox = unionBox;
    line.confidence = sumConf / static_cast<float>(tokens.size());
    line.text = text;
    line.tokens = std::move(tokens);

    return line;
}

std::vector<OcrLine> LineGrouper::groupTokensIntoLines(
    const std::vector<OcrToken>& tokens,
    double yToleranceRatio
) {
    if (tokens.empty()) return {};

    std::vector<OcrToken> sortedTokens = tokens;
    // Initial sort by Y coordinate, then X
    std::sort(sortedTokens.begin(), sortedTokens.end(), [](const OcrToken& a, const OcrToken& b) {
        if (std::abs(a.bbox.y - b.bbox.y) > 4) {
            return a.bbox.y < b.bbox.y;
        }
        return a.bbox.x < b.bbox.x;
    });

    std::vector<std::vector<OcrToken>> lineBuckets;

    for (const auto& token : sortedTokens) {
        if (token.text.empty() && token.bbox.empty()) continue;

        int tokCenterY = token.bbox.y + token.bbox.height / 2;
        int tokHeight = std::max(1, token.bbox.height);

        bool assigned = false;
        for (auto& bucket : lineBuckets) {
            if (bucket.empty()) continue;

            // Calculate average center Y and average height of bucket
            int sumY = 0;
            int sumH = 0;
            for (const auto& bt : bucket) {
                sumY += (bt.bbox.y + bt.bbox.height / 2);
                sumH += bt.bbox.height;
            }
            int avgCenterY = sumY / static_cast<int>(bucket.size());
            int avgHeight = std::max(1, sumH / static_cast<int>(bucket.size()));

            int threshold = static_cast<int>(std::max(tokHeight, avgHeight) * yToleranceRatio);
            if (std::abs(tokCenterY - avgCenterY) <= threshold) {
                bucket.push_back(token);
                assigned = true;
                break;
            }
        }

        if (!assigned) {
            lineBuckets.push_back({token});
        }
    }

    std::vector<OcrLine> resultLines;
    resultLines.reserve(lineBuckets.size());

    for (auto& bucket : lineBuckets) {
        resultLines.push_back(buildLineFromTokens(std::move(bucket)));
    }

    return ReadingOrder::sortLines(resultLines);
}

} // namespace edmo::ocr::representation
