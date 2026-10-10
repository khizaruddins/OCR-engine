#pragma once

#include <edmo/ocr/OcrBlock.hpp>
#include <edmo/ocr/OcrLine.hpp>
#include <edmo/ocr/OcrToken.hpp>
#include <edmo/ocr/representation/RepresentationConfig.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace edmo::ocr::representation {

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

} // namespace edmo::ocr::representation
