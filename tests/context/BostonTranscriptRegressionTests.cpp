#include <gtest/gtest.h>
#include <edmo/ocr/context/LlmContextBuilder.hpp>
#include <edmo/ocr/representation/DocumentRepresentationEngine.hpp>

#include <filesystem>
#include <fstream>

using namespace edmo::ocr;
using namespace edmo::ocr::context;

TEST(BostonTranscriptRegressionTests, PreservesBostonCollegeTranscriptWithZeroLoss) {
    std::vector<std::filesystem::path> candidates = {
        "output/documents/boston_transcript/raw_ocr.json",
        "../output/documents/boston_transcript/raw_ocr.json",
        "/home/khizaruddin/projects/fullapp/EDMO-OCR-DI/EDMO C++ OCR Engine/OCR Layer/output/documents/boston_transcript/raw_ocr.json",
        "output/raw_json/boston_transcript.json",
        "../output/raw_json/boston_transcript.json",
        "/home/khizaruddin/projects/fullapp/EDMO-OCR-DI/EDMO C++ OCR Engine/OCR Layer/output/raw_json/boston_transcript.json"
    };

    std::filesystem::path rawJsonPath;
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p)) {
            rawJsonPath = p;
            break;
        }
    }

    if (rawJsonPath.empty()) {
        GTEST_SKIP() << "Test fixture not found in candidate paths";
    }

    representation::DocumentRepresentationEngine reprEngine;
    auto edrRes = reprEngine.transform(rawJsonPath);
    ASSERT_TRUE(bool(edrRes)) << (edrRes ? "" : edrRes.error().message);

    LlmContextBuilder builder;
    auto buildRes = builder.build(edrRes.value());
    ASSERT_TRUE(bool(buildRes)) << (buildRes ? "" : buildRes.error().message);

    const auto& context = buildRes.value().context;
    const auto& report = buildRes.value().report;

    EXPECT_EQ(context.pageCount, 2);
    EXPECT_EQ(report.pageCoverage.represented, 2);
    EXPECT_EQ(report.lossAnalysis.unaccountedLossItems, 0);
    EXPECT_GE(report.contextQualityScore, 90.0);
}
