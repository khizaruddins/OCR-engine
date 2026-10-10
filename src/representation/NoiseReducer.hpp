#pragma once

#include <ocr/OcrBlock.hpp>
#include <ocr/OcrLine.hpp>
#include <ocr/OcrToken.hpp>
#include <ocr/representation/RepresentationConfig.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace ocr::representation {

class NoiseReducer {
public:
    static bool isNoiseToken(
        const OcrToken& token,
        const NoiseReductionConfig& config = {}
    );

    static bool isNoiseLine(
        const OcrLine& line,
        const NoiseReductionConfig& config = {}
    );

    static bool isNoiseBlock(
        const OcrBlock& block,
        const NoiseReductionConfig& config = {}
    );

    static std::vector<OcrLine> filterLines(
        const std::vector<OcrLine>& lines,
        const NoiseReductionConfig& config = {}
    );

    static std::vector<OcrBlock> filterBlocks(
        const std::vector<OcrBlock>& blocks,
        const NoiseReductionConfig& config = {}
    );
};

} // namespace ocr::representation
