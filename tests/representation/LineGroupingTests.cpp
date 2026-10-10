#include <gtest/gtest.h>
#include "../../src/representation/LineGrouper.hpp"

using namespace edmo::ocr;
using namespace edmo::ocr::representation;

TEST(LineGroupingTests, BuildLineFromTokens) {
    std::vector<OcrToken> tokens = {
        {"CS101", 0.94f, {150, 500, 80, 30}},
        {"Introduction", 0.92f, {250, 500, 160, 30}},
        {"to", 0.95f, {420, 500, 30, 30}},
        {"CS", 0.96f, {460, 500, 40, 30}}
    };

    OcrLine line = LineGrouper::buildLineFromTokens(tokens);

    EXPECT_EQ(line.text, "CS101 Introduction to CS");
    EXPECT_NEAR(line.confidence, (0.94f + 0.92f + 0.95f + 0.96f) / 4.0f, 0.01f);
    EXPECT_EQ(line.bbox.x, 150);
    EXPECT_EQ(line.bbox.right(), 500);
}

TEST(LineGroupingTests, GroupDispersedTokensIntoLines) {
    std::vector<OcrToken> tokens = {
        // Line 2 tokens
        {"Introduction", 0.90f, {200, 200, 100, 25}},
        {"CS101", 0.95f, {50, 202, 80, 24}},
        {"3.0", 0.93f, {400, 199, 40, 26}},
        {"A", 0.97f, {500, 201, 20, 25}},

        // Line 1 tokens
        {"OFFICIAL", 0.98f, {50, 100, 120, 30}},
        {"TRANSCRIPT", 0.96f, {180, 101, 150, 29}}
    };

    auto lines = LineGrouper::groupTokensIntoLines(tokens);

    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines[0].text, "OFFICIAL TRANSCRIPT");
    EXPECT_EQ(lines[1].text, "CS101 Introduction 3.0 A");
}
