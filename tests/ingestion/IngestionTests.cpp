#include <gtest/gtest.h>
#include <edmo/ocr/OcrEngine.hpp>
#include <edmo/ocr/ErrorCode.hpp>
#include <edmo/ocr/ingestion/DocumentIngestor.hpp>
#include <edmo/ocr/ingestion/NativeTextExtractor.hpp>
#include <edmo/ocr/ingestion/PageClassifier.hpp>
#include <edmo/ocr/ingestion/PageComplexityAnalyzer.hpp>
#include <edmo/ocr/ingestion/PdfInspector.hpp>
#include <edmo/ocr/ingestion/PreflightReport.hpp>
#include <edmo/ocr/ingestion/ProcessingStrategy.hpp>
#include <edmo/ocr/representation/DocumentRepresentationEngine.hpp>
#include <edmo/ocr/representation/EdrJsonSerializer.hpp>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace edmo::ocr;
using namespace edmo::ocr::ingestion;

#ifndef EDMO_TEST_FIXTURES_DIR
#define EDMO_TEST_FIXTURES_DIR "tests/fixtures"
#endif

class IngestionTests : public ::testing::Test {
protected:
    void SetUp() override {
        fixturesDir_ = std::filesystem::path(EDMO_TEST_FIXTURES_DIR);
    }

    std::filesystem::path fixturesDir_;
};

TEST_F(IngestionTests, PdfInspectorValidatesAndInspectsPdf) {
    auto pdfPath = fixturesDir_ / "multi-page.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdfPath));

    PdfInspector inspector(IngestionConfig{});
    auto res = inspector.inspect(pdfPath);
    ASSERT_TRUE(res.has_value()) << res.error().toString();

    const auto& summary = res.value();
    EXPECT_EQ(summary.schemaVersion, "edmo-preflight-0.2");
    EXPECT_EQ(summary.format, "PDF");
    EXPECT_EQ(summary.pageCount, 2);
    EXPECT_GT(summary.fileSizeBytes, 0);
    EXPECT_EQ(summary.pages.size(), 2);

    for (const auto& p : summary.pages) {
        EXPECT_GT(p.width, 0);
        EXPECT_GT(p.height, 0);
        EXPECT_GE(p.timing.preflightMs, 0.0);
    }
}

TEST_F(IngestionTests, PdfInspectorHandlesErrorCases) {
    PdfInspector inspector(IngestionConfig{});

    // Non-existent file
    auto resNonExistent = inspector.inspect("non_existent_file_xyz.pdf");
    ASSERT_FALSE(resNonExistent.has_value());
    EXPECT_EQ(resNonExistent.error().code, ErrorCode::FileNotFound);

    // Empty file
    auto emptyPath = std::filesystem::temp_directory_path() / "empty_ingest_test.pdf";
    std::ofstream(emptyPath.string()).close();
    auto resEmpty = inspector.inspect(emptyPath);
    ASSERT_FALSE(resEmpty.has_value());
    EXPECT_EQ(resEmpty.error().code, ErrorCode::FileEmpty);
    std::filesystem::remove(emptyPath);

    // Unsupported format (file exists but has unsupported extension)
    auto docxPath = std::filesystem::temp_directory_path() / "test.docx";
    {
        std::ofstream f(docxPath.string());
        f << "dummy data";
    }
    auto resUnsupported = inspector.inspect(docxPath);
    ASSERT_FALSE(resUnsupported.has_value());
    EXPECT_EQ(resUnsupported.error().code, ErrorCode::UnsupportedFormat);
    std::filesystem::remove(docxPath);
}

TEST_F(IngestionTests, PageClassifierClassifiesDeterministicTypes) {
    PageClassifier classifier(IngestionConfig{});

    // Digital text layer
    PageInspection digitalPage;
    digitalPage.pageNumber = 1;
    digitalPage.width = 612;
    digitalPage.height = 792;
    digitalPage.textLayer.exists = true;
    digitalPage.textLayer.meaningful = true;
    digitalPage.textLayer.characterCount = 1200;
    digitalPage.textLayer.wordCount = 220;
    digitalPage.textLayer.printableCharRatio = 0.98;
    digitalPage.textLayer.alphabeticRatio = 0.85;
    digitalPage.textLayer.qualityScore = 0.95;
    digitalPage.images.count = 0;
    digitalPage.images.coverageRatio = 0.0;

    auto classifiedDigital = classifier.classifyPage(digitalPage);
    EXPECT_EQ(classifiedDigital, PageInputType::DigitalText);

    // Scanned image layer
    PageInspection scannedPage;
    scannedPage.pageNumber = 1;
    scannedPage.width = 612;
    scannedPage.height = 792;
    scannedPage.textLayer.exists = false;
    scannedPage.textLayer.meaningful = false;
    scannedPage.textLayer.characterCount = 0;
    scannedPage.textLayer.wordCount = 0;
    scannedPage.textLayer.qualityScore = 0.0;
    scannedPage.images.count = 1;
    scannedPage.images.coverageRatio = 0.95;

    auto classifiedScanned = classifier.classifyPage(scannedPage);
    EXPECT_EQ(classifiedScanned, PageInputType::ScannedImage);

    // Empty page
    PageInspection emptyPage;
    emptyPage.pageNumber = 1;
    emptyPage.width = 612;
    emptyPage.height = 792;
    emptyPage.textLayer.exists = false;
    emptyPage.textLayer.meaningful = false;
    emptyPage.textLayer.characterCount = 0;
    emptyPage.textLayer.wordCount = 0;
    emptyPage.images.count = 0;
    emptyPage.images.coverageRatio = 0.0;

    auto classifiedEmpty = classifier.classifyPage(emptyPage);
    EXPECT_EQ(classifiedEmpty, PageInputType::Empty);
}

TEST_F(IngestionTests, ProcessingStrategyDispatchesCorrectly) {
    ProcessingStrategy engine(IngestionConfig{});
    PageInspection pi;

    EXPECT_EQ(engine.determineStrategy(PageInputType::DigitalText, pi), ProcessingStrategyType::NativeText);
    EXPECT_EQ(engine.determineStrategy(PageInputType::ScannedImage, pi), ProcessingStrategyType::Ocr);
    EXPECT_EQ(engine.determineStrategy(PageInputType::Empty, pi), ProcessingStrategyType::SkipEmpty);
    EXPECT_EQ(engine.determineStrategy(PageInputType::Corrupted, pi), ProcessingStrategyType::Error);

    // Force OCR override
    IngestionConfig forceOcrConfig;
    forceOcrConfig.forceOcr = true;
    ProcessingStrategy forceOcrEngine(forceOcrConfig);
    EXPECT_EQ(forceOcrEngine.determineStrategy(PageInputType::DigitalText, pi), ProcessingStrategyType::Ocr);

    // Force Native Text override
    IngestionConfig forceNativeConfig;
    forceNativeConfig.forceNativeText = true;
    ProcessingStrategy forceNativeEngine(forceNativeConfig);
    pi.textLayer.characterCount = 100;
    EXPECT_EQ(forceNativeEngine.determineStrategy(PageInputType::ScannedImage, pi), ProcessingStrategyType::NativeText);
}

TEST_F(IngestionTests, PageComplexityAnalyzerCalculatesScores) {
    PageComplexityAnalyzer analyzer;

    OcrPageResult page;
    page.width = 1000;
    page.height = 1000;

    OcrBlock block;
    block.bbox = BoundingBox(100, 100, 400, 200);

    OcrLine line;
    line.bbox = BoundingBox(100, 100, 400, 30);

    OcrToken token;
    token.text = "ComplexityTest";
    token.bbox = BoundingBox(100, 100, 100, 30);
    token.confidence = 0.99f;
    line.tokens.push_back(token);

    block.lines.push_back(line);
    page.blocks.push_back(block);

    auto complexity = analyzer.analyze(page);
    EXPECT_EQ(complexity.level, PageComplexityLevel::Simple);
    EXPECT_GT(complexity.score, 0.0);
    EXPECT_LE(complexity.score, 1.0);
}

TEST_F(IngestionTests, PreflightReportSerializesAndRoundtrips) {
    PreflightSummary summary;
    summary.schemaVersion = "edmo-preflight-0.2";
    summary.fileName = "test.pdf";
    summary.format = "PDF";
    summary.fileSizeBytes = 4096;
    summary.pageCount = 1;
    summary.documentType = DocumentLevelType::DigitalText;

    PageInspection pi;
    pi.pageNumber = 1;
    pi.width = 612;
    pi.height = 792;
    pi.inputType = PageInputType::DigitalText;
    pi.strategy = ProcessingStrategyType::NativeText;
    pi.complexity.level = PageComplexityLevel::Simple;
    pi.complexity.score = 0.2;
    summary.pages.push_back(pi);

    summary.performance.preflightTimeMs = 1.25;

    std::string jsonStr = PreflightReport::serialize(summary, true);
    ASSERT_FALSE(jsonStr.empty());

    nlohmann::json j = nlohmann::json::parse(jsonStr);
    EXPECT_EQ(j["schema_version"], "edmo-preflight-0.2");
    EXPECT_EQ(j["document"]["page_count"], 1);
    EXPECT_EQ(j["classification"]["document_type"], "DIGITAL_TEXT");
    EXPECT_EQ(j["pages"][0]["processing_strategy"], "NATIVE_TEXT");
    EXPECT_EQ(j["pages"][0]["input_type"], "DIGITAL_TEXT");
}

TEST_F(IngestionTests, EndToEndAdaptivePipelinePopulatesManifestAndMetadata) {
    auto pdfPath = fixturesDir_ / "transcript-like.pdf";
    ASSERT_TRUE(std::filesystem::exists(pdfPath));

    OcrConfig config = OcrConfig::getProfile("transcript");
    config.pdf.dpi = 150;
    OcrEngine engine(config);

    auto res = engine.process(pdfPath);
    ASSERT_TRUE(res.has_value()) << res.error().toString();

    const auto& result = res.value();
    EXPECT_EQ(result.engine.version, "0.2.0");
    EXPECT_FALSE(result.documentClassification.empty());
    EXPECT_FALSE(result.strategy.empty());
    EXPECT_GT(result.timing.preflightTimeMs, 0.0);
    EXPECT_EQ(result.preflight.pageCount, 2);

    for (const auto& p : result.pages) {
        EXPECT_FALSE(p.sourceType.empty());
        EXPECT_FALSE(p.processingStrategy.empty());
        EXPECT_FALSE(p.pageClassification.empty());
        EXPECT_GT(p.timing.totalMs, 0.0);
    }

    // Verify EDR Representation Manifest
    representation::RepresentationConfig reprConfig;
    representation::DocumentRepresentationEngine reprEngine(reprConfig);
    auto reprRes = reprEngine.transform(result);
    ASSERT_TRUE(reprRes.has_value()) << reprRes.error().toString();

    nlohmann::json edrJson = representation::EdrJsonSerializer::toFullJson(reprRes.value());
    EXPECT_EQ(edrJson["schema_version"], "edr-0.2");
    ASSERT_TRUE(edrJson.contains("processing_manifest"));
    EXPECT_EQ(edrJson["processing_manifest"]["engine"], "edmo-ocr");
    EXPECT_EQ(edrJson["processing_manifest"]["version"], "0.2.0");
    EXPECT_EQ(edrJson["processing_manifest"]["document"]["pages"], 2);
}
