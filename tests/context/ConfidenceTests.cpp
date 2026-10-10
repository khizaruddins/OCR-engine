#include <gtest/gtest.h>
#include <ocr/context/LlmContextBuilder.hpp>
#include <ocr/representation/SemanticTable.hpp>
#include "../../src/context/TableCompactor.hpp"
#include "../../src/context/SemanticInventoryBuilder.hpp"
#include "../../src/context/LossAnalyzer.hpp"

using namespace ocr;
using namespace ocr::context;
using namespace ocr::representation;

TEST(ConfidenceTests, PreservesLowConfidenceAndCriticalWarnings) {
    SemanticTable tbl;
    tbl.id = "tbl_conf";
    tbl.caption = "Grades";

    SemanticColumn c1, c2;
    c1.name = "Course"; c2.name = "Grade";
    tbl.columns = {c1, c2};

    SemanticRow r1;
    SemanticCell cell1, cell2;
    cell1.rawText = "MATH201"; cell1.confidence = 0.95f;
    cell2.rawText = "B+"; cell2.confidence = 0.52f; // Critical low confidence < 0.60
    r1.cells = {cell1, cell2};
    tbl.rows.push_back(r1);

    SemanticDocument doc;
    doc.tables.push_back(tbl);
    auto inventory = SemanticInventoryBuilder::build(doc);

    LlmContextConfig cfg;
    cfg.confidence.preserveBelow = 0.80f;
    cfg.confidence.criticalBelow = 0.60f;

    auto compactTbl = TableCompactor::compactTable(tbl, cfg, inventory);

    ASSERT_EQ(compactTbl.lowConfidenceCells.size(), 1);
    EXPECT_EQ(compactTbl.lowConfidenceCells[0].colIndex, 1);
    EXPECT_EQ(compactTbl.lowConfidenceCells[0].cellText, "B+");
    EXPECT_FLOAT_EQ(compactTbl.lowConfidenceCells[0].confidence, 0.52f);
    EXPECT_EQ(compactTbl.lowConfidenceCells[0].warning, "critical_low_confidence");
}

TEST(ConfidenceTests, OutputAnnotationsGreaterThanSourceItemsNeverExceeds100Percent) {
    SemanticDocument doc;
    doc.metadata.pageCount = 1;

    SemanticPage page;
    page.pageNumber = 1;

    // Create 10 low-confidence blocks
    for (int i = 0; i < 10; ++i) {
        RepresentationBlock b;
        b.id = "block_" + std::to_string(i);
        b.text = "Item " + std::to_string(i);
        b.type = BlockType::Paragraph;
        b.confidence.mean = 0.55f;
        b.confidence.lowConfidenceTokenCount = 1;
        page.blocks.push_back(b);
    }
    doc.pages.push_back(page);

    auto inventory = SemanticInventoryBuilder::build(doc);
    EXPECT_EQ(inventory.sourceLowConfidenceIds.size(), 10);

    // Mark all 10 as compacted/represented
    for (int i = 0; i < 10; ++i) {
        inventory.recordFate("block_" + std::to_string(i), ItemFate::Compacted, "Compacted");
    }

    LlmContext ctx;
    ctx.pagesPresent = {1};
    LlmContextSection sec;
    sec.id = "sec_1";
    for (int i = 0; i < 10; ++i) {
        sec.paragraphs.push_back("Item " + std::to_string(i));
    }
    ctx.sections.push_back(sec);

    // Add multiple warnings / annotations in output (e.g. 20 annotations)
    for (int i = 0; i < 20; ++i) {
        ctx.warnings.push_back("Warning annotation " + std::to_string(i));
    }

    auto report = LossAnalyzer::analyze(doc, ctx, inventory);

    EXPECT_EQ(report.confidencePreservation.sourceLowConfidenceItems, 10);
    EXPECT_EQ(report.confidencePreservation.sourceItemsWithPreservedEvidence, 10);
    EXPECT_EQ(report.confidencePreservation.outputConfidenceAnnotations, 20);
    EXPECT_LE(report.confidencePreservation.preservationPercentage, 100.0);
    EXPECT_DOUBLE_EQ(report.confidencePreservation.preservationPercentage, 100.0);
}

TEST(ConfidenceTests, ZeroLowConfidenceItemsYields100PercentPreservation) {
    SemanticDocument doc;
    doc.metadata.pageCount = 1;

    SemanticPage page;
    page.pageNumber = 1;
    RepresentationBlock b;
    b.id = "b1";
    b.text = "High Confidence Item";
    b.type = BlockType::Paragraph;
    b.confidence.mean = 0.98f;
    b.confidence.lowConfidenceTokenCount = 0;
    page.blocks.push_back(b);
    doc.pages.push_back(page);

    auto inventory = SemanticInventoryBuilder::build(doc);
    EXPECT_EQ(inventory.sourceLowConfidenceIds.size(), 0);

    inventory.recordFate("b1", ItemFate::Compacted, "Compacted");

    LlmContext ctx;
    ctx.pagesPresent = {1};
    LlmContextSection sec;
    sec.id = "sec_1";
    sec.paragraphs.push_back("High Confidence Item");
    ctx.sections.push_back(sec);

    auto report = LossAnalyzer::analyze(doc, ctx, inventory);

    EXPECT_EQ(report.confidencePreservation.sourceLowConfidenceItems, 0);
    EXPECT_EQ(report.confidencePreservation.sourceItemsWithPreservedEvidence, 0);
    EXPECT_DOUBLE_EQ(report.confidencePreservation.preservationPercentage, 100.0);
}
