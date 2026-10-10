#include <gtest/gtest.h>
#include "../../src/representation/TextNormalizer.hpp"
#include "../../src/representation/NoiseReducer.hpp"

using namespace edmo::ocr;
using namespace edmo::ocr::representation;

TEST(TextNormalizationTests, BasicTrimmingAndCollapsing) {
    EXPECT_EQ(TextNormalizer::trim("   CS101   "), "CS101");
    EXPECT_EQ(TextNormalizer::collapseWhitespace("John    Smith"), "John Smith");
    EXPECT_EQ(TextNormalizer::toUpper("Bachelor of science"), "BACHELOR OF SCIENCE");
    EXPECT_TRUE(TextNormalizer::iequals("CS101", "cs101"));
    EXPECT_TRUE(TextNormalizer::icontains("BOSTON COLLEGE", "college"));
}

TEST(TextNormalizationTests, NoiseReducerTests) {
    NoiseReductionConfig config;
    config.removeIsolatedSymbols = true;

    OcrToken pipeToken{"|", 0.1f, {10, 10, 5, 20}};
    OcrToken validToken{"CS101", 0.95f, {50, 10, 80, 20}};

    EXPECT_TRUE(NoiseReducer::isNoiseToken(pipeToken, config));
    EXPECT_FALSE(NoiseReducer::isNoiseToken(validToken, config));
}
