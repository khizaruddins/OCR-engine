#include "SemanticChunker.hpp"

namespace ocr::context {

size_t SemanticChunker::estimateJsonTokens(const nlohmann::json& j) {
    std::string dumped = j.dump();
    return dumped.size() / 4;
}

void SemanticChunker::applyBudgetAndChunking(
    LlmContext& context,
    const LlmContextConfig& config,
    SemanticInventory& inventory
) {
    (void)inventory;
    nlohmann::json baseJson = context.toJson(config.mode == ContextMode::Traceable);
    size_t totalTokens = estimateJsonTokens(baseJson);
    context.estimatedTokens = totalTokens;

    if (!config.chunking.enabled || totalTokens <= config.tokenBudget) {
        context.isChunked = false;
        return;
    }

    // Budget exceeded -> Generate semantic chunks
    context.isChunked = true;
    context.chunks.clear();

    // 1. Build Global Context Summary
    std::map<std::string, std::string> globalSummary;
    globalSummary["document_type"] = context.documentType;
    globalSummary["page_count"] = std::to_string(context.pageCount);
    if (context.student.count("student_name")) globalSummary["student_name"] = context.student.at("student_name");
    if (context.student.count("student_id")) globalSummary["student_id"] = context.student.at("student_id");
    if (context.institution.count("name")) globalSummary["institution_name"] = context.institution.at("name");
    if (context.academic.count("degree")) globalSummary["degree"] = context.academic.at("degree");
    if (context.academic.count("major")) globalSummary["major"] = context.academic.at("major");

    // 2. Metadata & Profile Chunk
    LlmContextChunk profileChunk;
    profileChunk.chunkId = "chunk_profile_overview";
    profileChunk.chunkType = "profile_overview";
    profileChunk.pageStart = 1;
    profileChunk.pageEnd = 1;
    profileChunk.globalContextSummary = globalSummary;

    for (const auto& sec : context.sections) {
        if (sec.type == "student_information" || sec.type == "institution_information" || sec.type == "academic_information") {
            profileChunk.sections.push_back(sec);
        }
    }

    profileChunk.estimatedTokens = estimateJsonTokens(profileChunk.toJson(config.mode == ContextMode::Traceable));
    context.chunks.push_back(std::move(profileChunk));

    // 3. Table Chunks (Partition large tables row-wise without splitting rows)
    size_t tableChunkCounter = 1;
    for (const auto& tbl : context.tables) {
        size_t maxRows = config.chunking.maxRowsPerTableChunk > 0 ? config.chunking.maxRowsPerTableChunk : 50;

        if (tbl.rows.size() <= maxRows) {
            LlmContextChunk tblChunk;
            tblChunk.chunkId = "chunk_table_" + std::to_string(tableChunkCounter++);
            tblChunk.chunkType = tbl.tableType;
            tblChunk.pageStart = tbl.pageStart;
            tblChunk.pageEnd = tbl.pageEnd;
            tblChunk.globalContextSummary = globalSummary;
            tblChunk.tables.push_back(tbl);
            tblChunk.estimatedTokens = estimateJsonTokens(tblChunk.toJson(config.mode == ContextMode::Traceable));
            context.chunks.push_back(std::move(tblChunk));
        } else {
            // Split table across multiple chunks
            for (size_t startRow = 0; startRow < tbl.rows.size(); startRow += maxRows) {
                size_t endRow = std::min(startRow + maxRows, tbl.rows.size());

                LlmContextTable subTbl;
                subTbl.id = tbl.id + "_part_" + std::to_string(startRow / maxRows + 1);
                subTbl.title = tbl.title + " (Rows " + std::to_string(startRow + 1) + "-" + std::to_string(endRow) + ")";
                subTbl.tableType = tbl.tableType;
                subTbl.pageStart = tbl.pageStart;
                subTbl.pageEnd = tbl.pageEnd;
                subTbl.headers = tbl.headers;
                subTbl.sourceRegion = tbl.sourceRegion;

                for (size_t r = startRow; r < endRow; ++r) {
                    subTbl.rows.push_back(tbl.rows[r]);
                }

                // Copy low confidence cell annotations for this slice
                for (const auto& annot : tbl.lowConfidenceCells) {
                    if (annot.rowIndex >= startRow && annot.rowIndex < endRow) {
                        CellConfidenceAnnotation subAnnot = annot;
                        subAnnot.rowIndex = annot.rowIndex - startRow;
                        subTbl.lowConfidenceCells.push_back(std::move(subAnnot));
                    }
                }

                LlmContextChunk tblChunk;
                tblChunk.chunkId = "chunk_table_" + std::to_string(tableChunkCounter++);
                tblChunk.chunkType = tbl.tableType;
                tblChunk.pageStart = tbl.pageStart;
                tblChunk.pageEnd = tbl.pageEnd;
                tblChunk.globalContextSummary = globalSummary;
                tblChunk.tables.push_back(std::move(subTbl));
                tblChunk.estimatedTokens = estimateJsonTokens(tblChunk.toJson(config.mode == ContextMode::Traceable));
                context.chunks.push_back(std::move(tblChunk));
            }
        }
    }

    // 4. Other Sections Chunks
    for (const auto& sec : context.sections) {
        if (sec.type != "student_information" && sec.type != "institution_information" && sec.type != "academic_information") {
            LlmContextChunk otherChunk;
            otherChunk.chunkId = "chunk_section_" + sec.id;
            otherChunk.chunkType = sec.type;
            otherChunk.pageStart = sec.pageStart;
            otherChunk.pageEnd = sec.pageEnd;
            otherChunk.globalContextSummary = globalSummary;
            otherChunk.sections.push_back(sec);
            otherChunk.estimatedTokens = estimateJsonTokens(otherChunk.toJson(config.mode == ContextMode::Traceable));
            context.chunks.push_back(std::move(otherChunk));
        }
    }

    // Update chunk indices
    for (size_t i = 0; i < context.chunks.size(); ++i) {
        context.chunks[i].chunkIndex = i + 1;
        context.chunks[i].totalChunks = context.chunks.size();
    }
}

} // namespace ocr::context
