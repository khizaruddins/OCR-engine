#include "TableCompactor.hpp"
#include "../representation/TextNormalizer.hpp"

namespace edmo::ocr::context {

LlmContextTable TableCompactor::compactTable(
    const representation::SemanticTable& table,
    const LlmContextConfig& config,
    SemanticInventory& inventory
) {
    LlmContextTable outTbl;
    outTbl.id = table.id;
    outTbl.title = table.caption;
    outTbl.tableType = table.tableType;
    outTbl.pageStart = table.pageStart;
    outTbl.pageEnd = table.pageEnd;
    outTbl.sourceRegion = "page_" + std::to_string(table.pageStart) + "_bbox[" +
                          std::to_string(table.source.bbox.x) + "," +
                          std::to_string(table.source.bbox.y) + "," +
                          std::to_string(table.source.bbox.width) + "," +
                          std::to_string(table.source.bbox.height) + "]";

    inventory.recordFate(table.id, ItemFate::Structured, "Transformed into compact table matrix");

    // 1. Extract Column Headers
    for (size_t c = 0; c < table.columns.size(); ++c) {
        std::string colName = table.columns[c].name;
        if (colName.empty()) {
            colName = table.columns[c].canonicalName;
        }
        if (colName.empty()) {
            colName = "Column_" + std::to_string(c + 1);
        }
        outTbl.headers.push_back(std::move(colName));
    }

    // 2. Extract Rows and Cells
    for (size_t rIdx = 0; rIdx < table.rows.size(); ++rIdx) {
        const auto& row = table.rows[rIdx];
        if (row.isHeader) {
            inventory.recordFate(table.id + "_row_" + std::to_string(rIdx), ItemFate::Metadata, "Table column header line");
            continue;
        }

        std::vector<std::string> rowValues;
        std::string rowId = table.id + "_row_" + std::to_string(rIdx);

        if (!row.cells.empty()) {
            for (size_t cIdx = 0; cIdx < row.cells.size(); ++cIdx) {
                const auto& cell = row.cells[cIdx];
                std::string cellText = representation::TextNormalizer::collapseWhitespace(
                    representation::TextNormalizer::trim(cell.text())
                );
                rowValues.push_back(cellText);

                std::string cellId = rowId + "_c" + std::to_string(cIdx);
                inventory.recordFate(cellId, ItemFate::Compacted, "Compacted cell value");

                // Check low confidence
                float cellConf = cell.confidence;
                if (cellConf > 0.0f && cellConf < config.confidence.preserveBelow) {
                    CellConfidenceAnnotation annot;
                    annot.rowIndex = outTbl.rows.size();
                    annot.colIndex = cIdx;
                    annot.cellText = cellText;
                    annot.confidence = cellConf;
                    if (cellConf < config.confidence.criticalBelow) {
                        annot.warning = "critical_low_confidence";
                    }
                    outTbl.lowConfidenceCells.push_back(std::move(annot));
                }
            }
        } else if (!row.canonicalFields.empty()) {
            // Fallback from canonical fields if cells vector wasn't populated
            size_t cIdx = 0;
            if (!outTbl.headers.empty()) {
                for (const auto& h : outTbl.headers) {
                    std::string val;
                    if (row.canonicalFields.count(h)) {
                        val = row.canonicalFields.at(h);
                    }
                    std::string cellText = representation::TextNormalizer::collapseWhitespace(
                        representation::TextNormalizer::trim(val)
                    );
                    rowValues.push_back(cellText);
                    std::string cellId = rowId + "_c" + std::to_string(cIdx);
                    inventory.recordFate(cellId, ItemFate::Compacted, "Compacted cell value");
                    cIdx++;
                }
            } else {
                for (const auto& [k, v] : row.canonicalFields) {
                    std::string cellText = representation::TextNormalizer::collapseWhitespace(
                        representation::TextNormalizer::trim(v)
                    );
                    rowValues.push_back(cellText);
                    std::string cellId = rowId + "_c" + std::to_string(cIdx);
                    inventory.recordFate(cellId, ItemFate::Compacted, "Compacted cell value");
                    cIdx++;
                }
            }
        }

        if (!rowValues.empty()) {
            outTbl.rows.push_back(std::move(rowValues));
            inventory.recordFate(rowId, ItemFate::Structured, "Structured table row");
        }
    }

    return outTbl;
}

std::vector<LlmContextTable> TableCompactor::compactTables(
    const std::vector<representation::SemanticTable>& tables,
    const LlmContextConfig& config,
    SemanticInventory& inventory
) {
    std::vector<LlmContextTable> res;
    res.reserve(tables.size());
    for (const auto& tbl : tables) {
        res.push_back(compactTable(tbl, config, inventory));
    }
    return res;
}

} // namespace edmo::ocr::context
