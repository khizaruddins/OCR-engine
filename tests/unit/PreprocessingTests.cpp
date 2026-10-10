#include <gtest/gtest.h>
#include <edmo/ocr/Image.hpp>
#include <edmo/ocr/OcrConfig.hpp>
#include "../../src/preprocessing/Binarization.hpp"
#include "../../src/preprocessing/Contrast.hpp"
#include "../../src/preprocessing/Denoise.hpp"
#include "../../src/preprocessing/Deskew.hpp"
#include "../../src/preprocessing/ImagePreprocessor.hpp"
#include "../../src/preprocessing/Sharpen.hpp"
#include <edmo/ocr/preprocessing/TableGridDetector.hpp>

#include <opencv2/imgproc.hpp>

using namespace edmo::ocr;

class PreprocessingTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a synthetic 200x200 3-channel test image with text-like stripes
        testMat_ = cv::Mat(200, 200, CV_8UC3, cv::Scalar(240, 240, 240));
        // Add horizontal black bars simulating text lines
        for (int y = 30; y < 170; y += 20) {
            cv::line(testMat_, cv::Point(20, y), cv::Point(180, y), cv::Scalar(0, 0, 0), 2);
        }
        testImage_ = Image(testMat_.clone());
    }

    cv::Mat testMat_;
    Image testImage_;
};

TEST_F(PreprocessingTests, DenoiseGaussian) {
    PreprocessingConfig config;
    config.denoise = true;
    config.denoiseMethod = DenoiseMethod::Gaussian;
    config.denoiseKernelSize = 3;

    auto res = Denoise::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().width(), 200);
    EXPECT_EQ(res.value().height(), 200);
    EXPECT_FALSE(res.value().empty());
}

TEST_F(PreprocessingTests, DenoiseMedian) {
    PreprocessingConfig config;
    config.denoise = true;
    config.denoiseMethod = DenoiseMethod::Median;
    config.denoiseKernelSize = 3;

    auto res = Denoise::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().width(), 200);
    EXPECT_EQ(res.value().height(), 200);
}

TEST_F(PreprocessingTests, ContrastClahe) {
    PreprocessingConfig config;
    config.contrast = true;
    config.contrastMethod = ContrastMethod::Clahe;
    config.claheClipLimit = 2.0;

    auto res = Contrast::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().channels(), 1); // CLAHE converts to grayscale
    EXPECT_EQ(res.value().width(), 200);
}

TEST_F(PreprocessingTests, ContrastEqualize) {
    PreprocessingConfig config;
    config.contrast = true;
    config.contrastMethod = ContrastMethod::HistogramEqualization;

    auto res = Contrast::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().channels(), 1);
}

TEST_F(PreprocessingTests, BinarizationOtsu) {
    PreprocessingConfig config;
    config.binarization = true;
    config.binarizationMethod = BinarizationMethod::Otsu;

    auto res = Binarization::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().channels(), 1);

    // Binarized output should contain only 0 and 255 values
    const cv::Mat& m = res.value().mat();
    for (int r = 0; r < m.rows; ++r) {
        for (int c = 0; c < m.cols; ++c) {
            uint8_t val = m.at<uint8_t>(r, c);
            EXPECT_TRUE(val == 0 || val == 255);
        }
    }
}

TEST_F(PreprocessingTests, BinarizationAdaptive) {
    PreprocessingConfig config;
    config.binarization = true;
    config.binarizationMethod = BinarizationMethod::AdaptiveGaussian;
    config.adaptiveBlockSize = 11;

    auto res = Binarization::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().channels(), 1);
}

TEST_F(PreprocessingTests, DeskewAngleDetection) {
    // Rotate test image by 4.0 degrees
    cv::Point2f center(100.0f, 100.0f);
    cv::Mat rot = cv::getRotationMatrix2D(center, 4.0, 1.0);
    cv::Mat skewedMat;
    cv::warpAffine(testMat_, skewedMat, rot, cv::Size(200, 200), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    cv::Mat gray;
    cv::cvtColor(skewedMat, gray, cv::COLOR_BGR2GRAY);

    double detectedAngle = Deskew::detectSkewAngle(gray, DeskewMethod::HoughLines, 10.0);
    // Detected angle magnitude should be near 4.0 degrees (within +/- 1.0 degree tolerance for synthetic sample)
    EXPECT_NEAR(std::abs(detectedAngle), 4.0, 1.0);
}

TEST_F(PreprocessingTests, SharpenFilter) {
    PreprocessingConfig config;
    config.sharpen = true;
    config.sharpenStrength = 1.0;

    auto res = Sharpen::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().width(), 200);
}

TEST_F(PreprocessingTests, EndToEndPreprocessorPipeline) {
    ImagePreprocessor preprocessor;
    PreprocessingConfig config;
    config.enabled = true;
    config.grayscale = true;
    config.denoise = true;
    config.contrast = true;
    config.binarization = false;
    config.deskew = true;
    config.orientation = false;

    DebugContext debugCtx;
    debugCtx.enabled = false;

    auto res = preprocessor.process(testImage_, config, debugCtx);
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res.value().image.empty());
    EXPECT_EQ(res.value().image.channels(), 1);
    EXPECT_GE(res.value().durationMs, 0.0);
}

TEST_F(PreprocessingTests, BinarizationSauvola) {
    PreprocessingConfig config;
    config.binarization = true;
    config.binarizationMethod = BinarizationMethod::Sauvola;
    config.adaptiveBlockSize = 15;

    auto res = Binarization::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().width(), 200);
    EXPECT_EQ(res.value().height(), 200);
    EXPECT_EQ(res.value().channels(), 1);
}

TEST_F(PreprocessingTests, BinarizationBlackHat) {
    PreprocessingConfig config;
    config.binarization = true;
    config.binarizationMethod = BinarizationMethod::BlackHat;
    config.adaptiveBlockSize = 15;

    auto res = Binarization::process(testImage_, config);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().width(), 200);
    EXPECT_EQ(res.value().height(), 200);
    EXPECT_EQ(res.value().channels(), 1);
}

TEST_F(PreprocessingTests, TableGridDetectionSynthetic) {
    // Generate a 400x300 image with a 3x3 table grid
    cv::Mat tableMat(300, 400, CV_8UC3, cv::Scalar(255, 255, 255));
    // Outer boundary
    cv::rectangle(tableMat, cv::Rect(40, 40, 320, 200), cv::Scalar(0, 0, 0), 2);
    // Horizontal divider lines
    cv::line(tableMat, cv::Point(40, 100), cv::Point(360, 100), cv::Scalar(0, 0, 0), 2);
    cv::line(tableMat, cv::Point(40, 160), cv::Point(360, 160), cv::Scalar(0, 0, 0), 2);
    // Vertical divider lines
    cv::line(tableMat, cv::Point(140, 40), cv::Point(140, 240), cv::Scalar(0, 0, 0), 2);
    cv::line(tableMat, cv::Point(250, 40), cv::Point(250, 240), cv::Scalar(0, 0, 0), 2);

    Image img(tableMat);
    TableGridDetectorConfig config;
    config.minTableWidth = 100;
    config.minTableHeight = 60;
    config.horizontalLineKernelDivisor = 15;
    config.verticalLineKernelDivisor = 15;

    auto res = TableGridDetector::detect(img, config);
    ASSERT_TRUE(res.has_value());
    const auto& tables = res.value();
    ASSERT_GE(tables.size(), 1u);
    EXPECT_TRUE(tables.front().hasPhysicalBorders);
    EXPECT_GE(tables.front().rowCount, 2);
    EXPECT_GE(tables.front().colCount, 2);
    EXPECT_GE(tables.front().cells.size(), 4u);

    // Verify debug visualization
    cv::Mat visual = TableGridDetector::renderDebugVisualization(tableMat, tables);
    EXPECT_FALSE(visual.empty());
    EXPECT_EQ(visual.channels(), 3);
}

