#include "ReadingOrder.hpp"

#include <algorithm>
#include <cmath>

namespace ocr::representation {

void ReadingOrder::sortTokensInLine(OcrLine& line) {
    if (line.tokens.size() <= 1) return;

    std::sort(line.tokens.begin(), line.tokens.end(), [](const OcrToken& a, const OcrToken& b) {
        if (a.bbox.x != b.bbox.x) {
            return a.bbox.x < b.bbox.x;
        }
        return a.bbox.y < b.bbox.y;
    });
}

std::vector<OcrLine> ReadingOrder::sortLines(const std::vector<OcrLine>& lines) {
    if (lines.size() <= 1) return lines;

    std::vector<OcrLine> sorted = lines;
    for (auto& line : sorted) {
        sortTokensInLine(line);
    }

    std::sort(sorted.begin(), sorted.end(), [](const OcrLine& a, const OcrLine& b) {
        // If lines significantly overlap vertically (more than 50% of the smaller height), sort horizontally
        int overlapY = std::max(0, std::min(a.bbox.bottom(), b.bbox.bottom()) - std::max(a.bbox.top(), b.bbox.top()));
        int minH = std::min(std::max(1, a.bbox.height), std::max(1, b.bbox.height));
        if (overlapY > minH / 2) {
            return a.bbox.x < b.bbox.x;
        }
        return a.bbox.y < b.bbox.y;
    });

    return sorted;
}

std::vector<ReadingRegion> ReadingOrder::segmentRegions(
    const std::vector<OcrBlock>& blocks,
    int pageWidth,
    int pageHeight,
    const ReadingOrderConfig& config
) {
    if (blocks.empty()) return {};

    // If multi-column is disabled or page dimensions are invalid, return single region
    if (!config.enabled || !config.multiColumn || pageWidth <= 0 || pageHeight <= 0) {
        ReadingRegion region;
        region.id = 1;
        region.columnIndex = 0;
        region.blocks = blocks;
        return {region};
    }

    // 1. Detect Header / Footer boundaries
    // Top 10% or Bottom 10% can be candidates, but let's check actual distribution
    int headerLimit = static_cast<int>(pageHeight * 0.12);
    int footerLimit = static_cast<int>(pageHeight * 0.88);

    // 2. Check for multi-column layout in body
    // Collect body blocks
    std::vector<OcrBlock> headerBlocks;
    std::vector<OcrBlock> bodyBlocks;
    std::vector<OcrBlock> footerBlocks;

    for (const auto& b : blocks) {
        if (b.bbox.bottom() <= headerLimit) {
            headerBlocks.push_back(b);
        } else if (b.bbox.y >= footerLimit) {
            footerBlocks.push_back(b);
        } else {
            bodyBlocks.push_back(b);
        }
    }

    // Check for vertical gutter in body
    int midX = pageWidth / 2;
    int gutterMargin = static_cast<int>(pageWidth * config.columnGutterThresholdRatio);

    int leftCount = 0;
    int rightCount = 0;
    int straddlingCount = 0;

    for (const auto& b : bodyBlocks) {
        if (b.bbox.right() <= midX + gutterMargin / 2) {
            leftCount++;
        } else if (b.bbox.x >= midX - gutterMargin / 2) {
            rightCount++;
        } else {
            straddlingCount++;
        }
    }

    bool isTwoColumn = (leftCount >= 2 && rightCount >= 2 && straddlingCount == 0);

    std::vector<ReadingRegion> regions;
    int regionId = 1;

    if (!headerBlocks.empty()) {
        ReadingRegion hr;
        hr.id = regionId++;
        hr.columnIndex = -1;
        hr.blocks = headerBlocks;
        regions.push_back(std::move(hr));
    }

    if (isTwoColumn) {
        std::vector<OcrBlock> col1Blocks;
        std::vector<OcrBlock> col2Blocks;

        for (const auto& b : bodyBlocks) {
            if (b.bbox.right() <= midX + gutterMargin / 2) {
                col1Blocks.push_back(b);
            } else {
                col2Blocks.push_back(b);
            }
        }

        if (!col1Blocks.empty()) {
            ReadingRegion c1;
            c1.id = regionId++;
            c1.columnIndex = 0;
            c1.blocks = std::move(col1Blocks);
            regions.push_back(std::move(c1));
        }

        if (!col2Blocks.empty()) {
            ReadingRegion c2;
            c2.id = regionId++;
            c2.columnIndex = 1;
            c2.blocks = std::move(col2Blocks);
            regions.push_back(std::move(c2));
        }
    } else {
        if (!bodyBlocks.empty()) {
            ReadingRegion bodyReg;
            bodyReg.id = regionId++;
            bodyReg.columnIndex = 0;
            bodyReg.blocks = std::move(bodyBlocks);
            regions.push_back(std::move(bodyReg));
        }
    }

    if (!footerBlocks.empty()) {
        ReadingRegion fr;
        fr.id = regionId++;
        fr.columnIndex = 999;
        fr.blocks = footerBlocks;
        regions.push_back(std::move(fr));
    }

    return regions;
}

std::vector<OcrBlock> ReadingOrder::sortBlocks(
    const std::vector<OcrBlock>& blocks,
    int pageWidth,
    int pageHeight,
    const ReadingOrderConfig& config
) {
    if (blocks.size() <= 1) return blocks;

    auto regions = segmentRegions(blocks, pageWidth, pageHeight, config);
    std::vector<OcrBlock> sorted;
    sorted.reserve(blocks.size());

    for (auto& region : regions) {
        // Sort blocks inside region top-to-bottom
        std::sort(region.blocks.begin(), region.blocks.end(), [](const OcrBlock& a, const OcrBlock& b) {
            int overlapY = std::max(0, std::min(a.bbox.bottom(), b.bbox.bottom()) - std::max(a.bbox.top(), b.bbox.top()));
            int minH = std::min(std::max(1, a.bbox.height), std::max(1, b.bbox.height));
            if (overlapY > minH / 2) {
                return a.bbox.x < b.bbox.x;
            }
            return a.bbox.y < b.bbox.y;
        });

        for (auto& b : region.blocks) {
            b.lines = sortLines(b.lines);
            sorted.push_back(std::move(b));
        }
    }

    return sorted;
}

} // namespace ocr::representation
