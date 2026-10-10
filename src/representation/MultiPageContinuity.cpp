#include "MultiPageContinuity.hpp"

#include <algorithm>

namespace edmo::ocr::representation {

bool MultiPageContinuity::canMerge(const SemanticTable& a, const SemanticTable& b) {
    if (a.tableType != b.tableType) return false;
    if (a.pageEnd + 1 != b.pageStart) return false;
    if (a.tableType != "course_records") return false;

    // Check if column counts or names are compatible
    if (a.columns.empty() || b.columns.empty()) return true;

    int matchCount = 0;
    for (const auto& ca : a.columns) {
        for (const auto& cb : b.columns) {
            if (ca.canonicalName == cb.canonicalName) {
                matchCount++;
                break;
            }
        }
    }

    return matchCount >= 2;
}

std::vector<SemanticTable> MultiPageContinuity::mergeContinuousTables(
    std::vector<SemanticTable> tables,
    const TableConfig& config
) {
    if (!config.enabled || !config.mergeMultiPage || tables.size() <= 1) {
        return tables;
    }

    std::vector<SemanticTable> merged;

    for (auto& current : tables) {
        if (merged.empty()) {
            merged.push_back(std::move(current));
        } else {
            auto& prev = merged.back();
            if (canMerge(prev, current)) {
                // Merge current into prev
                prev.pageEnd = current.pageEnd;
                for (auto& r : current.rows) {
                    prev.rows.push_back(std::move(r));
                }
                prev.confidence.merge(current.confidence);
                // Source union
                prev.source.bbox = prev.source.bbox.unionWith(current.source.bbox);
            } else {
                merged.push_back(std::move(current));
            }
        }
    }

    return merged;
}

} // namespace edmo::ocr::representation
