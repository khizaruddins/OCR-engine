#pragma once

#include <edmo/ocr/OcrLine.hpp>
#include <edmo/ocr/OcrToken.hpp>

#include <vector>

namespace edmo::ocr::representation {

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

} // namespace edmo::ocr::representation
