#include <gtest/gtest.h>
#include <edmo/ocr/OcrEngine.hpp>
#include <edmo/ocr/ErrorCode.hpp>

#include <filesystem>

using namespace edmo::ocr;

#ifndef EDMO_TEST_FIXTURES_DIR
#define EDMO_TEST_FIXTURES_DIR "tests/fixtures"
#endif

class PdfTests : public ::testing::Test {
protected:
    void SetUp() override {
        fixturesDir_ = std::filesystem::path(EDMO_TEST_FIXTURES_DIR);
    }

    std::filesystem::path fixturesDir_;
};

TEST_F(PdfTests, MultiPagePdfProcessing) {
    auto pdfPath = fixturesDir_ / "multi-page.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdfPath)) << "Fixture missing: " << pdfPath;

    OcrConfig config = OcrConfig::getProfile("default");
    config.pdf.dpi = 150; // fast DPI for tests
    OcrEngine engine(config);

    auto res = engine.process(pdfPath);
    ASSERT_TRUE(res.has_value()) << "Failed with error: " << res.error().toString();

    const auto& result = res.value();
    EXPECT_EQ(result.document.type, "pdf");
    EXPECT_EQ(result.document.pageCount, 2);
    ASSERT_EQ(result.pages.size(), 2);

    EXPECT_EQ(result.pages[0].pageNumber, 1);
    EXPECT_EQ(result.pages[1].pageNumber, 2);

    EXPECT_GT(result.pages[0].width, 0);
    EXPECT_GT(result.pages[0].height, 0);
    EXPECT_GT(result.pages[1].width, 0);
    EXPECT_GT(result.pages[1].height, 0);

    EXPECT_FALSE(result.pages[0].rawText.empty());
    EXPECT_FALSE(result.pages[1].rawText.empty());

    EXPECT_GT(result.timing.pdfRenderTimeMs, 0.0);
    EXPECT_GT(result.timing.ocrTimeMs, 0.0);
}

TEST_F(PdfTests, MaxPageLimitEnforcement) {
    auto pdfPath = fixturesDir_ / "multi-page.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdfPath));

    OcrConfig config = OcrConfig::getProfile("default");
    config.pdf.maxPages = 1; // 2-page document should exceed limit 1

    OcrEngine engine(config);
    auto res = engine.process(pdfPath);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code, ErrorCode::FileTooLarge);
}
