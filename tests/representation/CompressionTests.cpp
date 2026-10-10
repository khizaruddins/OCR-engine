#include <gtest/gtest.h>
#include "../../src/representation/SemanticCompressor.hpp"
#include <edmo/ocr/representation/DocumentRepresentationEngine.hpp>

using namespace edmo::ocr;
using namespace edmo::ocr::representation;

TEST(CompressionTests, ComputeMetricsAndTokenEstimate) {
    OcrResult ocr;
    ocr.document.id = "doc_123";
    ocr.document.pageCount = 1;

    OcrPageResult p;
    p.pageNumber = 1;
    p.width = 1000;
    p.height = 1000;

    OcrBlock b;
    b.bbox = {100, 100, 500, 30};
    b.lines = {
        {"CS101 Introduction to CS 3 A", 0.95f, {100, 100, 500, 30}, {
            {"CS101", 0.95f, {100, 100, 60, 30}},
            {"Introduction", 0.95f, {170, 100, 150, 30}},
            {"to", 0.95f, {330, 100, 20, 30}},
            {"CS", 0.95f, {360, 100, 30, 30}},
            {"3", 0.95f, {400, 100, 20, 30}},
            {"A", 0.95f, {430, 100, 20, 30}}
        }}
    };
    p.blocks.push_back(b);
    ocr.pages.push_back(p);

    DocumentRepresentationEngine engine;
    auto res = engine.transform(ocr);
    ASSERT_TRUE(res.has_value());

    const auto& doc = res.value();
    EXPECT_GT(doc.compression.rawBytes, 0);
    EXPECT_GT(doc.compression.edrFullBytes, 0);
    EXPECT_GT(doc.compression.edrLlmBytes, 0);
    EXPECT_GT(doc.compression.rawToLlmRatio, 1.0);
    EXPECT_GE(doc.compression.textPreservationPercentage, 90.0);
    EXPECT_EQ(doc.compression.estimatedLlmTokens, doc.compression.edrLlmCharacters / 4);
}
