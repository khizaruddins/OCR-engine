#include <gtest/gtest.h>
#include <edmo/ocr/representation/ConfidenceSummary.hpp>
#include "../../src/representation/ConfidenceAggregator.hpp"

using namespace edmo::ocr::representation;

TEST(ConfidenceTests, SummaryCalculation) {
    ConfidenceSummary cs;
    cs.addScore(0.90f, 0.70f);
    cs.addScore(0.80f, 0.70f);
    cs.addScore(0.50f, 0.70f); // low confidence

    EXPECT_EQ(cs.tokenCount, 3);
    EXPECT_NEAR(cs.mean, (0.90f + 0.80f + 0.50f) / 3.0f, 0.01f);
    EXPECT_FLOAT_EQ(cs.minimum, 0.50f);
    EXPECT_FLOAT_EQ(cs.maximum, 0.90f);
    EXPECT_EQ(cs.lowConfidenceTokenCount, 1);
}

TEST(ConfidenceTests, SummaryMerging) {
    ConfidenceSummary cs1;
    cs1.addScore(0.90f);
    cs1.addScore(0.80f);

    ConfidenceSummary cs2;
    cs2.addScore(0.60f);
    cs2.addScore(0.50f);

    cs1.merge(cs2);

    EXPECT_EQ(cs1.tokenCount, 4);
    EXPECT_NEAR(cs1.mean, (0.90f + 0.80f + 0.60f + 0.50f) / 4.0f, 0.01f);
    EXPECT_FLOAT_EQ(cs1.minimum, 0.50f);
    EXPECT_FLOAT_EQ(cs1.maximum, 0.90f);
    EXPECT_EQ(cs1.lowConfidenceTokenCount, 2);
}
