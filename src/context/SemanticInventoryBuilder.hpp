#pragma once

#include <ocr/context/LlmContextModels.hpp>
#include <ocr/representation/SemanticDocument.hpp>

#include <set>
#include <vector>

namespace ocr::context {

struct SemanticInventory {
    int pageCount = 0;
    size_t totalBlocks = 0;
    size_t totalTables = 0;
    size_t totalRows = 0;
    size_t totalCells = 0;
    size_t totalSections = 0;
    size_t totalLowConfidenceItems = 0;
    size_t totalMeaningfulCharacters = 0;

    std::vector<ContentItem> items;
    std::map<std::string, size_t> itemIndexById;
    std::set<std::string> sourceLowConfidenceIds;

    void recordFate(const std::string& itemId, ItemFate fate, std::string reason = "") {
        auto it = itemIndexById.find(itemId);
        if (it != itemIndexById.end() && it->second < items.size()) {
            items[it->second].fate = fate;
            if (!reason.empty()) {
                items[it->second].fateReason = std::move(reason);
            }
        }
    }
};

class SemanticInventoryBuilder {
public:
    static SemanticInventory build(const representation::SemanticDocument& doc);
};

} // namespace ocr::context
