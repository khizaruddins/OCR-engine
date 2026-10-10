#include <gtest/gtest.h>
#include <edmo/ocr/context/LlmContextBuilder.hpp>

#include <filesystem>
#include <fstream>

using namespace edmo::ocr;
using namespace edmo::ocr::context;

TEST(SevenPageTranscriptRegressionTests, PreservesAllSevenPagesAndCoursesWithoutLoss) {
    std::vector<std::filesystem::path> candidates = {
        "output/documents/International_transcript-IND_Redacted_compressed/edr_full.json",
        "../output/documents/International_transcript-IND_Redacted_compressed/edr_full.json",
        "/home/khizaruddin/projects/fullapp/EDMO-OCR-DI/EDMO C++ OCR Engine/OCR Layer/output/documents/International_transcript-IND_Redacted_compressed/edr_full.json",
        "output/raw_json/International transcript-IND_Redacted_compressed_edr_full.json",
        "../output/raw_json/International transcript-IND_Redacted_compressed_edr_full.json",
        "/home/khizaruddin/projects/fullapp/EDMO-OCR-DI/EDMO C++ OCR Engine/OCR Layer/output/raw_json/International transcript-IND_Redacted_compressed_edr_full.json"
    };

    std::filesystem::path edrFullPath;
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p)) {
            edrFullPath = p;
            break;
        }
    }

    if (edrFullPath.empty()) {
        GTEST_SKIP() << "Test fixture not found in candidate paths";
    }

    LlmContextConfig cfg;
    cfg.tokenBudget = 12000;
    LlmContextBuilder builder(cfg);

    auto buildRes = builder.build(edrFullPath);
    ASSERT_TRUE(bool(buildRes)) << (buildRes ? "" : buildRes.error().message);

    const auto& context = buildRes.value().context;
    const auto& report = buildRes.value().report;

    // 1. Page Coverage Verification
    EXPECT_EQ(context.pageCount, 7);
    EXPECT_EQ(context.pagesPresent.size(), 7);
    EXPECT_EQ(report.pageCoverage.detected, 7);
    EXPECT_EQ(report.pageCoverage.represented, 7);
    EXPECT_TRUE(report.pageCoverage.pagesMissing.empty());
    EXPECT_DOUBLE_EQ(report.pageCoverage.coveragePercentage, 100.0);

    // 2. Section Coverage Verification
    EXPECT_EQ(report.sectionCoverage.sectionsDetected, 2);
    EXPECT_EQ(report.sectionCoverage.sectionsRepresented, 2);
    EXPECT_DOUBLE_EQ(report.sectionCoverage.coveragePercentage, 100.0);
    EXPECT_FALSE(report.sectionCoverage.sections.empty());

    // 3. Table Coverage Verification
    EXPECT_GT(report.tableCoverage.tablesDetected, 0);
    EXPECT_EQ(report.tableCoverage.tablesDetected, report.tableCoverage.tablesRepresented);
    EXPECT_GT(report.tableCoverage.rowsDetected, 0);
    EXPECT_EQ(report.tableCoverage.rowsDetected, report.tableCoverage.rowsRepresented);
    EXPECT_DOUBLE_EQ(report.tableCoverage.rowCoveragePercentage, 100.0);

    // 4. Confidence Preservation Verification (Never > 100%)
    EXPECT_GT(report.confidencePreservation.sourceLowConfidenceItems, 0);
    EXPECT_EQ(report.confidencePreservation.sourceItemsWithPreservedEvidence,
              report.confidencePreservation.sourceLowConfidenceItems);
    EXPECT_LE(report.confidencePreservation.preservationPercentage, 100.0);
    EXPECT_DOUBLE_EQ(report.confidencePreservation.preservationPercentage, 100.0);

    // 5. Zero Unaccounted Loss Verification
    EXPECT_EQ(report.lossAnalysis.unaccountedLossItems, 0)
        << "Audit detected unaccounted loss items!";
    EXPECT_EQ(report.status, "VERIFIED_ZERO_UNACCOUNTED_LOSS");

    // 6. Meaningful Token Volume Verification (NOT 65 tokens / 262 bytes!)
    EXPECT_GT(report.compression.estimatedLlmTokens, 1000)
        << "LLM Context dropped multi-page transcript content!";
    EXPECT_GT(report.compression.llmContextBytes, 4000);

    // 7. Quality Score Verification
    EXPECT_GE(report.contextQualityScore, 90.0);

    // 8. Deduplicated Document Header Verification
    EXPECT_FALSE(context.documentHeader.empty());
    EXPECT_TRUE(context.documentHeader.find("CVR COLLEGE OF ENGINEERING") != std::string::npos);
}
