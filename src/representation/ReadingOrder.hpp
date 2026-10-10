#pragma once

#include <ocr/OcrBlock.hpp>
#include <ocr/OcrLine.hpp>
#include <ocr/OcrResult.hpp>
#include <ocr/representation/RepresentationConfig.hpp>

#include <vector>

namespace ocr::representation {

struct ReadingRegion {
    int id = 0;
    int columnIndex = 0; // -1: header, 0: col 1, 1: col 2, ..., 999: footer
    BoundingBox bbox;
    std::vector<OcrBlock> blocks;
};

class ReadingOrder {
public:
    static std::vector<OcrBlock> sortBlocks(
        const std::vector<OcrBlock>& blocks,
        int pageWidth,
        int pageHeight,
        const ReadingOrderConfig& config = {}
    );

    static std::vector<OcrLine> sortLines(
        const std::vector<OcrLine>& lines
    );

    static void sortTokensInLine(
        OcrLine& line
    );

    static std::vector<ReadingRegion> segmentRegions(
        const std::vector<OcrBlock>& blocks,
        int pageWidth,
        int pageHeight,
        const ReadingOrderConfig& config = {}
    );
};

} // namespace ocr::representation
