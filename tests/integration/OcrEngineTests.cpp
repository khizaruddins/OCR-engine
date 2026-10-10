#include <gtest/gtest.h>
#include <ocr/OcrEngine.hpp>
#include <ocr/ErrorCode.hpp>

#include <filesystem>
#include <fstream>

using namespace ocr;

#ifndef OCR_TEST_FIXTURES_DIR
#define OCR_TEST_FIXTURES_DIR "tests/fixtures"
#endif

class OcrEngineTests : public ::testing::Test {
protected:
    void SetUp() override {
        fixturesDir_ = std::filesystem::path(OCR_TEST_FIXTURES_DIR);
    }

    std::filesystem::path fixturesDir_;
};

TEST_F(OcrEngineTests, ProcessSimpleImage) {
    auto imgPath = fixturesDir_ / "simple-text.png";
    ASSERT_TRUE(std::filesystem::exists(imgPath)) << "Fixture missing: " << imgPath;

    OcrConfig config = OcrConfig::getProfile("default");
    OcrEngine engine(config);

    auto res = engine.process(imgPath);
    ASSERT_TRUE(res.has_value()) << "Failed with error: " << res.error().toString();

    const auto& result = res.value();
    EXPECT_EQ(result.engine.name, "ocr-engine");
    EXPECT_EQ(result.engine.provider, "tesseract");
    EXPECT_EQ(result.document.type, "image");
    EXPECT_EQ(result.pages.size(), 1);

    const auto& p = result.pages[0];
    EXPECT_EQ(p.pageNumber, 1);
    EXPECT_GT(p.width, 0);
    EXPECT_GT(p.height, 0);
    EXPECT_GT(p.confidence, 0.70f);

    // Verify text contains expected student information
    EXPECT_TRUE(p.rawText.find("EDMO") != std::string::npos || p.rawText.find("Alex Morgan") != std::string::npos);
    EXPECT_FALSE(p.blocks.empty());

    // Verify tokens have valid bounding boxes
    bool hasValidTokenBox = false;
    for (const auto& block : p.blocks) {
        for (const auto& line : block.lines) {
            for (const auto& token : line.tokens) {
                if (!token.bbox.empty() && token.bbox.area() > 0) {
                    hasValidTokenBox = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(hasValidTokenBox);
}

TEST_F(OcrEngineTests, ProcessSkewedImageWithDeskew) {
    auto imgPath = fixturesDir_ / "skewed-text.png";
    ASSERT_TRUE(std::filesystem::exists(imgPath)) << "Fixture missing: " << imgPath;

    OcrConfig config = OcrConfig::getProfile("default");
    config.preprocessing.deskew = true;
    OcrEngine engine(config);

    auto res = engine.process(imgPath);
    ASSERT_TRUE(res.has_value()) << "Failed with error: " << res.error().toString();

    const auto& result = res.value();
    ASSERT_EQ(result.pages.size(), 1);
    const auto& p = result.pages[0];

    // Skew should have been detected and corrected
    EXPECT_GT(p.confidence, 0.70f);
    EXPECT_TRUE(p.rawText.find("CS101") != std::string::npos || p.rawText.find("HIGHER EDUCATION") != std::string::npos);
}

TEST_F(OcrEngineTests, ErrorHandlingNonExistentFile) {
    OcrEngine engine(OcrConfig::getProfile("default"));
    auto res = engine.process("non_existent_file_12345.png");

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code, ErrorCode::FileNotFound);
}

TEST_F(OcrEngineTests, ErrorHandlingEmptyFile) {
    auto tempEmpty = std::filesystem::temp_directory_path() / "empty_test.png";
    {
        std::ofstream ofs(tempEmpty); // creates 0-byte file
    }

    OcrEngine engine(OcrConfig::getProfile("default"));
    auto res = engine.process(tempEmpty);

    std::filesystem::remove(tempEmpty);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code, ErrorCode::FileEmpty);
}

TEST_F(OcrEngineTests, ErrorHandlingUnsupportedFormat) {
    auto tempUnsupported = std::filesystem::temp_directory_path() / "test.exe";
    {
        std::ofstream ofs(tempUnsupported);
        ofs << "dummy binary content";
    }

    OcrEngine engine(OcrConfig::getProfile("default"));
    auto res = engine.process(tempUnsupported);

    std::filesystem::remove(tempUnsupported);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code, ErrorCode::UnsupportedFormat);
}
