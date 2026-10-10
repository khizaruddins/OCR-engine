#include "SemanticInventoryBuilder.hpp"

namespace edmo::ocr::context {

SemanticInventory SemanticInventoryBuilder::build(const representation::SemanticDocument& doc) {
    SemanticInventory inv;
    inv.pageCount = doc.metadata.pageCount;
    inv.totalSections = doc.sections.size();
    inv.totalTables = doc.tables.size();

    // 1. Catalog Page Blocks
    for (const auto& page : doc.pages) {
        for (const auto& block : page.blocks) {
            inv.totalBlocks++;
            ContentItem item;
            item.id = block.id;
            item.type = "block_" + std::string(representation::blockTypeToString(block.type));
            item.pageNumber = page.pageNumber;
            item.text = block.text;
            item.confidence = block.confidence.mean;
            item.isLowConfidence = block.confidence.lowConfidenceTokenCount > 0 || block.confidence.mean < 0.80f;

            if (item.isLowConfidence) {
                inv.totalLowConfidenceItems++;
                inv.sourceLowConfidenceIds.insert(item.id);
            }

            inv.totalMeaningfulCharacters += block.text.size();
            inv.itemIndexById[item.id] = inv.items.size();
            inv.items.push_back(std::move(item));
        }
    }

    // 2. Catalog Tables, Rows, Cells
    for (const auto& tbl : doc.tables) {
        ContentItem tblItem;
        tblItem.id = tbl.id;
        tblItem.type = "table";
        tblItem.pageNumber = tbl.pageStart;
        tblItem.text = tbl.caption;
        tblItem.confidence = tbl.confidence.mean;
        inv.itemIndexById[tblItem.id] = inv.items.size();
        inv.items.push_back(std::move(tblItem));

        for (size_t rIdx = 0; rIdx < tbl.rows.size(); ++rIdx) {
            inv.totalRows++;
            const auto& row = tbl.rows[rIdx];
            std::string rowId = tbl.id + "_row_" + std::to_string(rIdx);

            ContentItem rowItem;
            rowItem.id = rowId;
            rowItem.type = "table_row";
            rowItem.pageNumber = row.source.pageNumber > 0 ? row.source.pageNumber : tbl.pageStart;
            rowItem.confidence = row.confidence;
            rowItem.isLowConfidence = row.confidence < 0.80f;

            std::string rowText;
            if (!row.cells.empty()) {
                for (size_t cIdx = 0; cIdx < row.cells.size(); ++cIdx) {
                    inv.totalCells++;
                    const auto& cell = row.cells[cIdx];
                    std::string cellId = rowId + "_c" + std::to_string(cIdx);
                    rowText += (cIdx > 0 ? " | " : "") + cell.text();

                    ContentItem cellItem;
                    cellItem.id = cellId;
                    cellItem.type = "table_cell";
                    cellItem.pageNumber = rowItem.pageNumber;
                    cellItem.text = cell.text();
                    cellItem.confidence = cell.confidence;
                    cellItem.isLowConfidence = cell.confidence > 0.0f && cell.confidence < 0.80f;
                    if (cellItem.isLowConfidence) {
                        inv.totalLowConfidenceItems++;
                        inv.sourceLowConfidenceIds.insert(cellItem.id);
                    }
                    inv.totalMeaningfulCharacters += cell.text().size();

                    inv.itemIndexById[cellItem.id] = inv.items.size();
                    inv.items.push_back(std::move(cellItem));
                }
            } else if (!row.canonicalFields.empty()) {
                size_t cIdx = 0;
                for (const auto& [k, v] : row.canonicalFields) {
                    inv.totalCells++;
                    std::string cellId = rowId + "_c" + std::to_string(cIdx);
                    rowText += (cIdx > 0 ? " | " : "") + v;

                    ContentItem cellItem;
                    cellItem.id = cellId;
                    cellItem.type = "table_cell";
                    cellItem.pageNumber = rowItem.pageNumber;
                    cellItem.text = v;
                    cellItem.confidence = row.confidence;
                    cellItem.isLowConfidence = row.confidence > 0.0f && row.confidence < 0.80f;
                    if (cellItem.isLowConfidence) {
                        inv.totalLowConfidenceItems++;
                        inv.sourceLowConfidenceIds.insert(cellItem.id);
                    }
                    inv.totalMeaningfulCharacters += v.size();

                    inv.itemIndexById[cellItem.id] = inv.items.size();
                    inv.items.push_back(std::move(cellItem));
                    cIdx++;
                }
            }

            rowItem.text = std::move(rowText);
            inv.itemIndexById[rowItem.id] = inv.items.size();
            inv.items.push_back(std::move(rowItem));
        }
    }

    return inv;
}

} // namespace edmo::ocr::context
