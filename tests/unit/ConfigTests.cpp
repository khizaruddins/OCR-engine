#include <gtest/gtest.h>
#include <edmo/ocr/OcrConfig.hpp>

using namespace edmo::ocr;

TEST(ConfigTests, ProfileLoading) {
    auto defaultConf = OcrConfig::getProfile("default");
    EXPECT_TRUE(defaultConf.preprocessing.enabled);
    EXPECT_TRUE(defaultConf.preprocessing.grayscale);
    EXPECT_FALSE(defaultConf.preprocessing.binarization);
    EXPECT_EQ(defaultConf.pdf.dpi, 300);

    auto transcriptConf = OcrConfig::getProfile("transcript");
    EXPECT_TRUE(transcriptConf.preprocessing.enabled);
    EXPECT_TRUE(transcriptConf.preprocessing.contrast);
    EXPECT_EQ(transcriptConf.preprocessing.contrastMethod, ContrastMethod::Clahe);
    EXPECT_FALSE(transcriptConf.preprocessing.binarization);
    EXPECT_EQ(transcriptConf.pdf.dpi, 300);

    auto cleanConf = OcrConfig::getProfile("clean-document");
    EXPECT_FALSE(cleanConf.preprocessing.denoise);
    EXPECT_FALSE(cleanConf.preprocessing.deskew);

    auto lowQualConf = OcrConfig::getProfile("low-quality-scan");
    EXPECT_TRUE(lowQualConf.preprocessing.binarization);
    EXPECT_TRUE(lowQualConf.preprocessing.sharpen);
}

TEST(ConfigTests, JsonParsingAndOverrides) {
    nlohmann::json j = {
        {"ocr", {
            {"language", "fra+eng"},
            {"psm", 6},
            {"device", "cpu"}
        }},
        {"pdf", {
            {"dpi", 400}
        }},
        {"preprocessing", {
            {"contrast_method", "equalize"},
            {"binarization", true},
            {"binarization_method", "otsu"}
        }}
    };

    auto res = OcrConfig::fromJson(j);
    ASSERT_TRUE(res.has_value());

    const auto& conf = res.value();
    EXPECT_EQ(conf.ocr.language, "fra+eng");
    EXPECT_EQ(conf.ocr.psm, 6);
    EXPECT_EQ(conf.pdf.dpi, 400);
    EXPECT_EQ(conf.preprocessing.contrastMethod, ContrastMethod::HistogramEqualization);
    EXPECT_TRUE(conf.preprocessing.binarization);
    EXPECT_EQ(conf.preprocessing.binarizationMethod, BinarizationMethod::Otsu);
}

TEST(ConfigTests, JsonRoundTrip) {
    OcrConfig original = OcrConfig::getProfile("transcript");
    nlohmann::json j = original.toJson();

    auto res = OcrConfig::fromJson(j);
    ASSERT_TRUE(res.has_value());

    const auto& parsed = res.value();
    EXPECT_EQ(parsed.ocr.provider, original.ocr.provider);
    EXPECT_EQ(parsed.ocr.language, original.ocr.language);
    EXPECT_EQ(parsed.pdf.dpi, original.pdf.dpi);
    EXPECT_EQ(parsed.preprocessing.claheClipLimit, original.preprocessing.claheClipLimit);
}

TEST(ConfigTests, InvalidJsonHandling) {
    // Malformed types that shouldn't crash
    nlohmann::json invalidJ = {
        {"pdf", {
            {"dpi", "not_a_number"}
        }}
    };

    auto res = OcrConfig::fromJson(invalidJ);
    // Should gracefully fail with InvalidConfiguration error or catch error
    if (!res.has_value()) {
        EXPECT_EQ(res.error().code, ErrorCode::InvalidConfiguration);
    }
}
