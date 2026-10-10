#include <gtest/gtest.h>
#include <edmo/ocr/OcrEngine.hpp>
#include <edmo/ocr/ErrorCode.hpp>
#include "../../src/output/JsonSerializer.hpp"

#include <filesystem>

using namespace edmo::ocr;

#ifndef EDMO_TEST_FIXTURES_DIR
#define EDMO_TEST_FIXTURES_DIR "tests/fixtures"
#endif

class TranscriptTests : public ::testing::Test {
protected:
    void SetUp() override {
        fixturesDir_ = std::filesystem::path(EDMO_TEST_FIXTURES_DIR);
    }

    std::filesystem::path fixturesDir_;
};

TEST_F(TranscriptTests, ProcessOfficialAcademicTranscript) {
    auto pdfPath = fixturesDir_ / "transcript-like.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdfPath)) << "Fixture missing: " << pdfPath;

    OcrConfig config = OcrConfig::getProfile("transcript");
    config.pdf.dpi = 200; // Efficient high-quality DPI for tests
    OcrEngine engine(config);

    auto res = engine.process(pdfPath);
    ASSERT_TRUE(res.has_value()) << "Failed with error: " << res.error().toString();

    const auto& result = res.value();
    EXPECT_EQ(result.schemaVersion, "0.1");
    EXPECT_EQ(result.engine.name, "edmo-ocr");
    EXPECT_EQ(result.document.type, "pdf");
    EXPECT_EQ(result.pages.size(), 2);
    EXPECT_GT(result.confidence, 0.75f);

    // Page 1 assertions
    const auto& p1 = result.pages[0];
    EXPECT_EQ(p1.pageNumber, 1);
    EXPECT_TRUE(p1.rawText.find("TRANSCRIPT") != std::string::npos);
    EXPECT_TRUE(p1.rawText.find("Emily Watson") != std::string::npos || p1.rawText.find("Watson") != std::string::npos);
    EXPECT_TRUE(p1.rawText.find("CS-101") != std::string::npos || p1.rawText.find("Programming") != std::string::npos);

    // Page 2 assertions
    const auto& p2 = result.pages[1];
    EXPECT_EQ(p2.pageNumber, 2);
    EXPECT_TRUE(p2.rawText.find("DEGREE") != std::string::npos || p2.rawText.find("BACHELOR") != std::string::npos);

    // Validate JSON Schema serialization
    std::string jsonOutput = JsonSerializer::serialize(result, OutputFormat::FullJson, true);
    ASSERT_FALSE(jsonOutput.empty());

    nlohmann::json j = nlohmann::json::parse(jsonOutput);
    EXPECT_EQ(j["schema_version"], "0.1");
    EXPECT_TRUE(j.contains("document"));
    EXPECT_TRUE(j.contains("pages"));
    EXPECT_TRUE(j.contains("processing"));
    EXPECT_EQ(j["pages"].size(), 2);
}
