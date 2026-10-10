#include <gtest/gtest.h>
#include <ocr/OcrEngine.hpp>
#include <ocr/BoundingBox.hpp>
#include "../../src/preprocessing/OrientationDetector.hpp"
#include "../../src/utils/CoordinateTransform.hpp"

#include <filesystem>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace ocr;

class OrientationTests : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(OrientationTests, CoordinateTransform0DegRoundTrip) {
    BoundingBox orig(100, 150, 200, 50);
    auto inverted = CoordinateTransform::invertBoundingBox(orig, 0, 1000, 2000);
    EXPECT_EQ(inverted.x, 100);
    EXPECT_EQ(inverted.y, 150);
    EXPECT_EQ(inverted.width, 200);
    EXPECT_EQ(inverted.height, 50);
}

TEST_F(OrientationTests, CoordinateTransform90DegClockwise) {
    // Original image: 1000 x 2000
    // Rotated 90° CW image: 2000 x 1000
    // Suppose a word is detected in the rotated image at (rx=200, ry=100, rw=50, rh=20)
    // x_orig = ry = 100
    // y_orig = H_orig - (rx + rw) = 2000 - 250 = 1750
    // w_orig = rh = 20
    // h_orig = rw = 50
    BoundingBox rotatedBox(200, 100, 50, 20);
    auto inverted = CoordinateTransform::invertBoundingBox(rotatedBox, 90, 1000, 2000);
    EXPECT_EQ(inverted.x, 100);
    EXPECT_EQ(inverted.y, 1750);
    EXPECT_EQ(inverted.width, 20);
    EXPECT_EQ(inverted.height, 50);
}

TEST_F(OrientationTests, CoordinateTransform180Deg) {
    // Original: 1000 x 2000
    // Rotated 180°: 1000 x 2000
    // Rotated box at (rx=100, ry=200, rw=50, rh=30)
    // x_orig = W_orig - (rx + rw) = 1000 - 150 = 850
    // y_orig = H_orig - (ry + rh) = 2000 - 230 = 1770
    BoundingBox rotatedBox(100, 200, 50, 30);
    auto inverted = CoordinateTransform::invertBoundingBox(rotatedBox, 180, 1000, 2000);
    EXPECT_EQ(inverted.x, 850);
    EXPECT_EQ(inverted.y, 1770);
    EXPECT_EQ(inverted.width, 50);
    EXPECT_EQ(inverted.height, 30);
}

TEST_F(OrientationTests, CoordinateTransform270Deg) {
    // Original: 1000 x 2000
    // Rotated 270° CW (90° CCW): 2000 x 1000
    // Rotated box at (rx=300, ry=150, rw=40, rh=20)
    // x_orig = W_orig - (ry + rh) = 1000 - 170 = 830
    // y_orig = rx = 300
    // w_orig = rh = 20
    // h_orig = rw = 40
    BoundingBox rotatedBox(300, 150, 40, 20);
    auto inverted = CoordinateTransform::invertBoundingBox(rotatedBox, 270, 1000, 2000);
    EXPECT_EQ(inverted.x, 830);
    EXPECT_EQ(inverted.y, 300);
    EXPECT_EQ(inverted.width, 20);
    EXPECT_EQ(inverted.height, 40);
}

TEST_F(OrientationTests, NuTranscriptRotatedScanRegression) {
    std::filesystem::path pdfPath = "/home/khizaruddin/Downloads/6a292e7d85303a760421cddb_6a35e62e529ae160f26c5a1f_NU-transcript-event-4644653_1781917230284.pdf";
    if (!std::filesystem::exists(pdfPath)) {
        GTEST_SKIP() << "NU regression PDF not found at path: " << pdfPath;
    }

    OcrConfig config;
    config.preprocessing.enabled = true;
    config.preprocessing.orientation = true;
    config.preprocessing.deskew = true;

    OcrEngine engine(config);
    auto res = engine.process(pdfPath);
    ASSERT_TRUE(res.has_value());

    const auto& doc = res.value();
    EXPECT_EQ(doc.pages.size(), 3);
    EXPECT_GT(doc.confidence, 0.70f); // Correct orientation should achieve high confidence > 70%

    // Verify all pages had 90° rotation applied
    for (const auto& p : doc.pages) {
        EXPECT_EQ(p.rotation, 90);
        EXPECT_GT(p.confidence, 0.70f);
        EXPECT_GT(p.blocks.size(), 0);
    }
}
