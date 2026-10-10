#pragma once

#include <ocr/OcrLine.hpp>
#include <ocr/representation/RepresentationBlock.hpp>
#include <ocr/representation/RepresentationConfig.hpp>

#include <string>
#include <vector>

namespace ocr::representation {

class BlockGrouper {
public:
    static std::vector<RepresentationBlock> groupLines(
        const std::vector<OcrLine>& lines,
        int pageNumber,
        int pageWidth,
        int pageHeight,
        const RepresentationConfig& config = {}
    );

    static BlockType classifyLine(
        const OcrLine& line,
        int pageHeight,
        std::string& outLabel,
        std::string& outValue
    );

    static bool parseLabelValue(
        const std::string& lineText,
        std::string& outLabel,
        std::string& outValue
    );

    static bool isHeading(
        const std::string& lineText
    );
};

} // namespace ocr::representation
