#include <gtest/gtest.h>
#include <edmo/ocr/OcrResult.hpp>
#include "../../src/output/JsonSerializer.hpp"

using namespace edmo::ocr;

TEST(SerializationTests, FullJsonSchemaCompliance) {
    OcrResult res;
    res.id = "proc_123456";
    res.schemaVersion = "0.1";
    res.engine.name = "edmo-ocr";
    res.engine.version = "0.1.0";
    res.engine.provider = "tesseract";

    res.document.id = "doc_789";
    res.document.source = "transcript.pdf";
    res.document.type = "pdf";
    res.document.pageCount = 1;
    res.document.fileSizeBytes = 102400;

    res.confidence = 0.95f;
    res.timing.pdfRenderTimeMs = 50.0;
    res.timing.preprocessTimeMs = 20.0;
    res.timing.ocrTimeMs = 150.0;
    res.timing.totalTimeMs = 220.0;

    OcrPageResult p;
    p.pageNumber = 1;
    p.width = 1600;
    p.height = 2200;
    p.rotation = 0;
    p.skewAngle = 0.0;
    p.confidence = 0.95f;
    p.rawText = "University of Document Intelligence\nCourse: CS101";
    p.normalizedText = "University of Document Intelligence\nCourse: CS101";

    OcrBlock block;
    block.text = "University of Document Intelligence";
    block.confidence = 0.98f;
    block.bbox = BoundingBox(100, 80, 600, 50);

    OcrLine line;
    line.text = "University of Document Intelligence";
    line.confidence = 0.98f;
    line.bbox = BoundingBox(100, 80, 600, 50);

    OcrToken t1;
    t1.text = "University";
    t1.confidence = 0.99f;
    t1.bbox = BoundingBox(100, 80, 150, 50);

    OcrToken t2;
    t2.text = "of";
    t2.confidence = 0.98f;
    t2.bbox = BoundingBox(260, 80, 40, 50);

    line.tokens.push_back(t1);
    line.tokens.push_back(t2);
    block.lines.push_back(line);
    p.blocks.push_back(block);

    res.pages.push_back(p);

    std::string jsonStr = JsonSerializer::serialize(res, OutputFormat::FullJson, true);
    ASSERT_FALSE(jsonStr.empty());

    nlohmann::json j = nlohmann::json::parse(jsonStr);

    EXPECT_EQ(j["schema_version"], "0.1");
    EXPECT_EQ(j["engine"]["name"], "edmo-ocr");
    EXPECT_EQ(j["engine"]["provider"], "tesseract");
    EXPECT_EQ(j["document"]["id"], "doc_789");
    EXPECT_EQ(j["document"]["type"], "pdf");
    EXPECT_EQ(j["pages"].size(), 1);
    EXPECT_EQ(j["pages"][0]["blocks"].size(), 1);
    EXPECT_EQ(j["pages"][0]["blocks"][0]["lines"][0]["tokens"].size(), 2);
    EXPECT_EQ(j["pages"][0]["blocks"][0]["lines"][0]["tokens"][0]["text"], "University");
}

TEST(SerializationTests, CompactJsonFormat) {
    OcrResult res;
    res.id = "proc_compact";
    res.schemaVersion = "0.1";
    res.engine.name = "edmo-ocr";
    res.document.source = "test.png";

    OcrPageResult p;
    p.pageNumber = 1;
    p.rawText = "Sample Text";
    p.normalizedText = "Sample Text";
    p.confidence = 0.92f;
    res.pages.push_back(p);

    std::string compactStr = JsonSerializer::serialize(res, OutputFormat::CompactJson, false);
    nlohmann::json j = nlohmann::json::parse(compactStr);

    EXPECT_TRUE(j.contains("pages"));
    EXPECT_EQ(j["pages"][0]["text"], "Sample Text");
    // In compact format, blocks hierarchy is omitted
    EXPECT_FALSE(j["pages"][0].contains("blocks"));
}

TEST(SerializationTests, PlainTextFormat) {
    OcrResult res;
    OcrPageResult p1;
    p1.pageNumber = 1;
    p1.normalizedText = "Page 1 Content";
    OcrPageResult p2;
    p2.pageNumber = 2;
    p2.normalizedText = "Page 2 Content";
    res.pages.push_back(p1);
    res.pages.push_back(p2);

    std::string text = JsonSerializer::serialize(res, OutputFormat::PlainText);
    EXPECT_TRUE(text.find("Page 1 Content") != std::string::npos);
    EXPECT_TRUE(text.find("Page 2 Content") != std::string::npos);
    EXPECT_TRUE(text.find("--- Page 1 ---") != std::string::npos);
}
