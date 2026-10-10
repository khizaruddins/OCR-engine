#include <gtest/gtest.h>
#include "../../src/representation/ReadingOrder.hpp"
#include <ocr/OcrBlock.hpp>
#include <ocr/OcrLine.hpp>

using namespace ocr;
using namespace ocr::representation;

TEST(ReadingOrderTests, TokenSortingInLine) {
    OcrLine line;
    line.tokens = {
        {"Science", 0.95f, {400, 100, 80, 20}},
        {"Computer", 0.95f, {250, 100, 120, 20}},
        {"to", 0.95f, {210, 100, 30, 20}},
        {"Introduction", 0.95f, {50, 100, 150, 20}}
    };

    ReadingOrder::sortTokensInLine(line);

    ASSERT_EQ(line.tokens.size(), 4);
    EXPECT_EQ(line.tokens[0].text, "Introduction");
    EXPECT_EQ(line.tokens[1].text, "to");
    EXPECT_EQ(line.tokens[2].text, "Computer");
    EXPECT_EQ(line.tokens[3].text, "Science");
}

TEST(ReadingOrderTests, SingleColumnLineSorting) {
    std::vector<OcrLine> lines = {
        {"Line 3", 0.9f, {100, 300, 200, 30}, {}},
        {"Line 1", 0.9f, {100, 100, 200, 30}, {}},
        {"Line 2", 0.9f, {100, 200, 200, 30}, {}}
    };

    auto sorted = ReadingOrder::sortLines(lines);

    ASSERT_EQ(sorted.size(), 3);
    EXPECT_EQ(sorted[0].text, "Line 1");
    EXPECT_EQ(sorted[1].text, "Line 2");
    EXPECT_EQ(sorted[2].text, "Line 3");
}

TEST(ReadingOrderTests, TwoColumnRegionSegmentation) {
    // Page: 1000x2000
    // Header at y=50
    // Col 1 at x=100, y=300..600
    // Col 2 at x=600, y=300..600
    // Footer at y=1900
    std::vector<OcrBlock> blocks = {
        {"Col 2 Line 1", 0.9f, {600, 300, 300, 40}, {}},
        {"Col 1 Line 1", 0.9f, {100, 300, 300, 40}, {}},
        {"Header Title", 0.9f, {200, 50, 600, 50}, {}},
        {"Col 1 Line 2", 0.9f, {100, 400, 300, 40}, {}},
        {"Col 2 Line 2", 0.9f, {600, 400, 300, 40}, {}},
        {"Footer Page 1", 0.9f, {400, 1900, 200, 40}, {}}
    };

    ReadingOrderConfig cfg;
    cfg.enabled = true;
    cfg.multiColumn = true;

    auto sorted = ReadingOrder::sortBlocks(blocks, 1000, 2000, cfg);

    ASSERT_EQ(sorted.size(), 6);
    EXPECT_EQ(sorted[0].text, "Header Title");
    EXPECT_EQ(sorted[1].text, "Col 1 Line 1");
    EXPECT_EQ(sorted[2].text, "Col 1 Line 2");
    EXPECT_EQ(sorted[3].text, "Col 2 Line 1");
    EXPECT_EQ(sorted[4].text, "Col 2 Line 2");
    EXPECT_EQ(sorted[5].text, "Footer Page 1");
}
