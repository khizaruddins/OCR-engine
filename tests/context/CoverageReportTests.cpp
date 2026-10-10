#include <gtest/gtest.h>
#include <ocr/context/LlmContextBuilder.hpp>
#include <ocr/representation/SemanticDocument.hpp>
#include <ocr/representation/EdrJsonSerializer.hpp>
#include "../../src/context/LossAnalyzer.hpp"
#include "../../src/context/SemanticInventoryBuilder.hpp"

using namespace ocr;
using namespace ocr::context;
using namespace ocr::representation;

TEST(CoverageReportTests, VerifiesZeroUnaccountedLossAndQualityScore) {
    SemanticDocument doc;
    doc.metadata.id = "cov_doc";
    doc.metadata.pageCount = 1;

    SemanticPage page;
    page.pageNumber = 1;

    RepresentationBlock b1;
    b1.id = "b1";
    b1.text = "Institution Name";
    b1.type = BlockType::Header;
    page.blocks.push_back(b1);

    RepresentationBlock b2;
    b2.id = "b2";
    b2.text = "Student Name: Bob";
    b2.type = BlockType::LabelValue;
    b2.label = "Student Name";
    b2.value = "Bob";
    page.blocks.push_back(b2);

    doc.pages.push_back(page);

    LlmContextBuilder builder;
    auto res = builder.build(doc);

    ASSERT_TRUE(bool(res));
    const auto& report = res.value().report;

    EXPECT_EQ(report.pageCoverage.represented, 1);
    EXPECT_EQ(report.pageCoverage.detected, 1);
    EXPECT_FLOAT_EQ(report.pageCoverage.coveragePercentage, 100.0f);
    EXPECT_EQ(report.lossAnalysis.unaccountedLossItems, 0);
    EXPECT_GE(report.contextQualityScore, 90.0);
    EXPECT_EQ(report.status, "VERIFIED_ZERO_UNACCOUNTED_LOSS");
}

TEST(CoverageReportTests, SectionTransformedIntoTableCountsAsRepresented) {
    SemanticDocument doc;
    doc.metadata.id = "sec_doc";
    doc.metadata.pageCount = 1;

    SemanticSection sec;
    sec.id = "section_course_history";
    sec.type = SectionType::CourseHistory;
    sec.title = "Academic Records";
    sec.pageStart = 1;
    sec.pageEnd = 1;

    SemanticTable tbl;
    tbl.id = "tbl_1";
    tbl.caption = "Semester 1";
    tbl.pageStart = 1;
    tbl.pageEnd = 1;
    sec.tables.push_back(tbl);
    doc.sections.push_back(sec);
    doc.tables.push_back(tbl);

    auto inventory = SemanticInventoryBuilder::build(doc);
    inventory.recordFate("tbl_1", ItemFate::Structured, "Structured as table");

    LlmContext ctx;
    ctx.pagesPresent = {1};
    LlmContextTable outTbl;
    outTbl.id = "tbl_1";
    outTbl.tableType = "course_history";
    outTbl.headers = {"Course", "Grade"};
    outTbl.rows = {{"CS101", "A"}};
    ctx.tables.push_back(outTbl);

    auto report = LossAnalyzer::analyze(doc, ctx, inventory);

    EXPECT_EQ(report.sectionCoverage.sectionsDetected, 1);
    EXPECT_EQ(report.sectionCoverage.sectionsRepresented, 1);
    EXPECT_DOUBLE_EQ(report.sectionCoverage.coveragePercentage, 100.0);
    ASSERT_EQ(report.sectionCoverage.sections.size(), 1);
    EXPECT_EQ(report.sectionCoverage.sections[0].fate, "STRUCTURED");
    EXPECT_TRUE(report.sectionCoverage.sections[0].represented);
}

TEST(CoverageReportTests, SectionDeduplicatedCountsAsRepresented) {
    SemanticDocument doc;
    doc.metadata.id = "sec_dedup";
    doc.metadata.pageCount = 2;

    SemanticSection sec;
    sec.id = "section_header";
    sec.type = SectionType::Header;
    sec.pageStart = 1;
    sec.pageEnd = 2;

    RepresentationBlock b1, b2;
    b1.id = "b1"; b1.text = "University Header";
    b2.id = "b2"; b2.text = "University Header";
    sec.blocks = {b1, b2};
    doc.sections.push_back(sec);

    SemanticPage p1, p2;
    p1.pageNumber = 1; p1.blocks = {b1};
    p2.pageNumber = 2; p2.blocks = {b2};
    doc.pages = {p1, p2};

    auto inventory = SemanticInventoryBuilder::build(doc);
    inventory.recordFate("b1", ItemFate::Deduplicated, "Header deduplicated");
    inventory.recordFate("b2", ItemFate::Deduplicated, "Header deduplicated");

    LlmContext ctx;
    ctx.pagesPresent = {1, 2};
    ctx.documentHeader = "University Header";

    auto report = LossAnalyzer::analyze(doc, ctx, inventory);

    EXPECT_EQ(report.sectionCoverage.sectionsDetected, 1);
    EXPECT_EQ(report.sectionCoverage.sectionsRepresented, 1);
    EXPECT_DOUBLE_EQ(report.sectionCoverage.coveragePercentage, 100.0);
    ASSERT_EQ(report.sectionCoverage.sections.size(), 1);
    EXPECT_EQ(report.sectionCoverage.sections[0].fate, "DEDUPLICATED");
    EXPECT_TRUE(report.sectionCoverage.sections[0].represented);
}

TEST(CoverageReportTests, MissingSectionTriggersWarningAndUnaccountedLoss) {
    SemanticDocument doc;
    doc.metadata.id = "sec_missing";
    doc.metadata.pageCount = 1;

    SemanticSection sec;
    sec.id = "section_lost";
    sec.type = SectionType::GradingScale;
    sec.pageStart = 1;
    sec.pageEnd = 1;

    RepresentationBlock b;
    b.id = "b_lost";
    b.text = "Grading Scale Legend";
    b.type = BlockType::Paragraph;
    sec.blocks = {b};
    doc.sections.push_back(sec);

    SemanticPage p;
    p.pageNumber = 1;
    p.blocks = {b};
    doc.pages.push_back(p);

    auto inventory = SemanticInventoryBuilder::build(doc);
    // Do NOT record any fate -> remains ItemFate::Unaccounted

    LlmContext ctx;
    ctx.pagesPresent = {1};
    // No tables, no sections matching the lost block

    auto report = LossAnalyzer::analyze(doc, ctx, inventory);

    EXPECT_EQ(report.sectionCoverage.sectionsDetected, 1);
    EXPECT_EQ(report.sectionCoverage.sectionsRepresented, 0);
    EXPECT_DOUBLE_EQ(report.sectionCoverage.coveragePercentage, 0.0);
    ASSERT_EQ(report.sectionCoverage.sections.size(), 1);
    EXPECT_EQ(report.sectionCoverage.sections[0].fate, "UNACCOUNTED_LOSS");
    EXPECT_FALSE(report.sectionCoverage.sections[0].represented);
    EXPECT_GT(report.lossAnalysis.unaccountedLossItems, 0);
    EXPECT_EQ(report.status, "ERROR_UNACCOUNTED_LOSS");
}

TEST(CoverageReportTests, ItemFateAccountingSumRule) {
    SemanticDocument doc;
    doc.metadata.id = "fate_sum_doc";
    doc.metadata.pageCount = 1;

    SemanticPage page;
    page.pageNumber = 1;

    // Create 100 blocks
    for (int i = 0; i < 100; ++i) {
        RepresentationBlock b;
        b.id = "b_" + std::to_string(i);
        b.text = "Block text " + std::to_string(i);
        b.type = BlockType::Paragraph;
        page.blocks.push_back(b);
    }
    doc.pages.push_back(page);

    auto inventory = SemanticInventoryBuilder::build(doc);
    EXPECT_EQ(inventory.items.size(), 100);

    // 30 compacted, 20 structured, 10 deduplicated, 10 preserved, 20 metadata, 5 noise, 5 chunked = 100
    int idx = 0;
    for (int i = 0; i < 30; ++i) inventory.recordFate("b_" + std::to_string(idx++), ItemFate::Compacted);
    for (int i = 0; i < 20; ++i) inventory.recordFate("b_" + std::to_string(idx++), ItemFate::Structured);
    for (int i = 0; i < 10; ++i) inventory.recordFate("b_" + std::to_string(idx++), ItemFate::Deduplicated);
    for (int i = 0; i < 10; ++i) inventory.recordFate("b_" + std::to_string(idx++), ItemFate::Preserved);
    for (int i = 0; i < 20; ++i) inventory.recordFate("b_" + std::to_string(idx++), ItemFate::Metadata);
    for (int i = 0; i < 5; ++i)  inventory.recordFate("b_" + std::to_string(idx++), ItemFate::Noise);
    for (int i = 0; i < 5; ++i)  inventory.recordFate("b_" + std::to_string(idx++), ItemFate::Chunked);

    LlmContext ctx;
    ctx.pagesPresent = {1};

    auto report = LossAnalyzer::analyze(doc, ctx, inventory);

    EXPECT_EQ(report.lossAnalysis.totalSourceItems, 100);
    EXPECT_EQ(report.lossAnalysis.compactedItems, 30);
    EXPECT_EQ(report.lossAnalysis.structuredItems, 20);
    EXPECT_EQ(report.lossAnalysis.deduplicatedItems, 10);
    EXPECT_EQ(report.lossAnalysis.preservedItems, 10);
    EXPECT_EQ(report.lossAnalysis.metadataItems, 20);
    EXPECT_EQ(report.lossAnalysis.noiseItems, 5);
    EXPECT_EQ(report.lossAnalysis.chunkedItems, 5);
    EXPECT_EQ(report.lossAnalysis.unaccountedLossItems, 0);

    size_t sum = report.lossAnalysis.preservedItems +
                 report.lossAnalysis.compactedItems +
                 report.lossAnalysis.structuredItems +
                 report.lossAnalysis.deduplicatedItems +
                 report.lossAnalysis.noiseItems +
                 report.lossAnalysis.metadataItems +
                 report.lossAnalysis.chunkedItems +
                 report.lossAnalysis.unaccountedLossItems;
    EXPECT_EQ(sum, 100);
}

TEST(CoverageReportTests, FullEdrBytesAndRatiosAreCalculatedAndNotNull) {
    SemanticDocument doc;
    doc.metadata.id = "doc_compression_test";
    doc.metadata.pageCount = 1;

    SemanticPage p;
    p.pageNumber = 1;
    RepresentationBlock b;
    b.id = "b1";
    b.text = "Boston College Registrar Office";
    b.type = BlockType::Header;
    p.blocks.push_back(b);
    doc.pages.push_back(p);

    nlohmann::json fullEdrJson = EdrJsonSerializer::toFullJson(doc);
    nlohmann::json rawJson = {{"ocr_tokens", 100}, {"text", "Boston College Registrar Office"}};

    LlmContextBuilder builder;
    auto res = builder.build(fullEdrJson, &rawJson);

    ASSERT_TRUE(bool(res));
    const auto& report = res.value().report;

    EXPECT_GT(report.compression.edrFullBytes, 0);
    EXPECT_GT(report.compression.estimatedFullTokens, 0);
    EXPECT_GT(report.compression.llmContextBytes, 0);
    EXPECT_GT(report.compression.estimatedLlmTokens, 0);
    EXPECT_GT(report.compression.rawBytes, 0);
    EXPECT_GT(report.compression.estimatedRawTokens, 0);

    EXPECT_TRUE(report.compression.rawToFullRatio.has_value());
    EXPECT_GT(*report.compression.rawToFullRatio, 0.0);

    EXPECT_TRUE(report.compression.fullToLlmRatio.has_value());
    EXPECT_GT(*report.compression.fullToLlmRatio, 0.0);

    EXPECT_TRUE(report.compression.rawToLlmRatio.has_value());
    EXPECT_GT(*report.compression.rawToLlmRatio, 0.0);

    EXPECT_EQ(report.textCoverage.unit, "characters");
}

TEST(CoverageReportTests, TableCellsAreDetectedAndRepresentedEqually) {
    SemanticDocument doc;
    doc.metadata.id = "table_cell_test";
    doc.metadata.pageCount = 1;

    SemanticTable tbl;
    tbl.id = "tbl_1";
    tbl.caption = "Semester Grades";
    SemanticColumn col1; col1.name = "Course"; col1.canonicalName = "course_code"; col1.columnIndex = 0;
    SemanticColumn col2; col2.name = "Grade"; col2.canonicalName = "grade"; col2.columnIndex = 1;
    tbl.columns = {col1, col2};

    SemanticRow r1;
    SemanticCell c1; c1.rawText = "CS101"; c1.confidence = 0.95f;
    SemanticCell c2; c2.rawText = "A"; c2.confidence = 0.98f;
    r1.cells = {c1, c2};
    r1.canonicalFields = {{"course_code", "CS101"}, {"grade", "A"}};

    SemanticRow r2;
    SemanticCell c3; c3.rawText = "MATH201"; c3.confidence = 0.92f;
    SemanticCell c4; c4.rawText = "B+"; c4.confidence = 0.88f;
    r2.cells = {c3, c4};
    r2.canonicalFields = {{"course_code", "MATH201"}, {"grade", "B+"}};

    tbl.rows = {r1, r2};
    doc.tables.push_back(tbl);

    LlmContextBuilder builder;
    auto res = builder.build(doc);

    ASSERT_TRUE(bool(res));
    const auto& report = res.value().report;

    EXPECT_EQ(report.tableCoverage.tablesDetected, 1);
    EXPECT_EQ(report.tableCoverage.tablesRepresented, 1);
    EXPECT_DOUBLE_EQ(report.tableCoverage.tableCoveragePercentage, 100.0);

    EXPECT_EQ(report.tableCoverage.rowsDetected, 2);
    EXPECT_EQ(report.tableCoverage.rowsRepresented, 2);
    EXPECT_DOUBLE_EQ(report.tableCoverage.rowCoveragePercentage, 100.0);

    EXPECT_EQ(report.tableCoverage.cellsDetected, 4);
    EXPECT_EQ(report.tableCoverage.cellsRepresented, 4);
    EXPECT_DOUBLE_EQ(report.tableCoverage.cellCoveragePercentage, 100.0);
}

TEST(CoverageReportTests, SectionTypeResolvesGenericSourceIdFallback) {
    SemanticDocument doc;
    doc.metadata.id = "sec_type_test";
    doc.metadata.pageCount = 1;

    SemanticSection s1;
    s1.id = "section_institution_info";
    s1.type = SectionType::Unknown; // Unknown type initially, should fallback from sourceId
    s1.fields = {{"name", "Boston College"}};
    doc.sections.push_back(s1);

    SemanticSection s2;
    s2.id = "section_academic_info";
    s2.type = SectionType::Unknown;
    s2.fields = {{"program", "Computer Science"}};
    doc.sections.push_back(s2);

    LlmContextBuilder builder;
    auto res = builder.build(doc);

    ASSERT_TRUE(bool(res));
    const auto& report = res.value().report;

    EXPECT_EQ(report.sectionCoverage.sectionsDetected, 2);
    EXPECT_EQ(report.sectionCoverage.sectionsRepresented, 2);
    EXPECT_DOUBLE_EQ(report.sectionCoverage.coveragePercentage, 100.0);

    ASSERT_EQ(report.sectionCoverage.sections.size(), 2);
    EXPECT_NE(report.sectionCoverage.sections[0].type, "unknown");
    EXPECT_EQ(report.sectionCoverage.sections[0].type, "institution_info");

    EXPECT_NE(report.sectionCoverage.sections[1].type, "unknown");
    EXPECT_EQ(report.sectionCoverage.sections[1].type, "academic_info");
}

