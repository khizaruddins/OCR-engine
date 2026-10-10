#include <gtest/gtest.h>
#include <ocr/context/LlmContextBuilder.hpp>
#include "../../src/context/SemanticChunker.hpp"
#include "../../src/context/SemanticInventoryBuilder.hpp"

using namespace ocr;
using namespace ocr::context;
using namespace ocr::representation;

TEST(ChunkingTests, ChunksTableWhilePreservingRowAndCellIntegrity) {
    LlmContext context;
    context.documentType = "transcript";
    context.pageCount = 3;
    context.student["student_name"] = "John Doe";
    context.institution["name"] = "State University";

    LlmContextTable tbl;
    tbl.id = "tbl_large";
    tbl.tableType = "course_records";
    tbl.pageStart = 1;
    tbl.pageEnd = 3;
    tbl.headers = {"Code", "Title", "Credits", "Grade"};

    for (int i = 1; i <= 60; ++i) {
        tbl.rows.push_back({"CS" + std::to_string(i), "Course Name " + std::to_string(i), "3", "A"});
    }
    context.tables.push_back(tbl);

    LlmContextConfig cfg;
    cfg.tokenBudget = 200; // Force chunking
    cfg.chunking.maxRowsPerTableChunk = 25;

    SemanticInventory inv;
    SemanticChunker::applyBudgetAndChunking(context, cfg, inv);

    EXPECT_TRUE(context.isChunked);
    EXPECT_GT(context.chunks.size(), 1);

    // Verify row integrity: sum of rows across table chunks must equal 60
    size_t totalChunkedRows = 0;
    for (const auto& ch : context.chunks) {
        if (!ch.tables.empty()) {
            for (const auto& t : ch.tables) {
                totalChunkedRows += t.rows.size();
                for (const auto& r : t.rows) {
                    EXPECT_EQ(r.size(), 4); // Cell integrity: each row has exactly 4 cells
                }
            }
        }
    }
    EXPECT_EQ(totalChunkedRows, 60);
}
