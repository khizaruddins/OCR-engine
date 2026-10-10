#include <gtest/gtest.h>
#include <ocr/representation/SemanticTable.hpp>
#include "../../src/context/TableCompactor.hpp"
#include "../../src/context/SemanticInventoryBuilder.hpp"

using namespace ocr;
using namespace ocr::context;
using namespace ocr::representation;

TEST(TableCompactionTests, CompactsVerboseTableInto2DMatrix) {
    SemanticTable tbl;
    tbl.id = "tbl_1";
    tbl.caption = "Fall 2024 Courses";
    tbl.pageStart = 1;
    tbl.pageEnd = 1;

    SemanticColumn c1, c2, c3;
    c1.name = "Course Code"; c1.canonicalName = "course_code";
    c2.name = "Course Title"; c2.canonicalName = "course_title";
    c3.name = "Credits"; c3.canonicalName = "credits";
    tbl.columns = {c1, c2, c3};

    SemanticRow r1;
    SemanticCell cell1, cell2, cell3;
    cell1.rawText = "CS101"; cell1.confidence = 0.98f;
    cell2.rawText = "Introduction to Programming"; cell2.confidence = 0.95f;
    cell3.rawText = "3"; cell3.confidence = 0.99f;
    r1.cells = {cell1, cell2, cell3};
    tbl.rows.push_back(r1);

    SemanticDocument doc;
    doc.tables.push_back(tbl);
    auto inventory = SemanticInventoryBuilder::build(doc);

    LlmContextConfig cfg;
    auto compactTbl = TableCompactor::compactTable(tbl, cfg, inventory);

    EXPECT_EQ(compactTbl.headers.size(), 3);
    EXPECT_EQ(compactTbl.rows.size(), 1);
    EXPECT_EQ(compactTbl.rows[0].size(), 3);
    EXPECT_EQ(compactTbl.rows[0][0], "CS101");
    EXPECT_EQ(compactTbl.rows[0][1], "Introduction to Programming");
    EXPECT_EQ(compactTbl.rows[0][2], "3");
    EXPECT_TRUE(compactTbl.lowConfidenceCells.empty());
}
