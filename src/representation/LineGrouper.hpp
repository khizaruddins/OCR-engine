#pragma once

#include <ocr/OcrLine.hpp>
#include <ocr/OcrToken.hpp>

#include <vector>

namespace ocr::representation {

class LineGrouper {
public:
    static std::vector<OcrLine> groupTokensIntoLines(
        const std::vector<OcrToken>& tokens,
        double yToleranceRatio = 0.50
    );

    static OcrLine buildLineFromTokens(
        std::vector<OcrToken> tokens
    );
};

} // namespace ocr::representation
