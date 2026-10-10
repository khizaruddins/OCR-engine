#pragma once

#include <ocr/context/LlmContextConfig.hpp>
#include <ocr/context/LlmContextModels.hpp>
#include <ocr/representation/SemanticTable.hpp>
#include "SemanticInventoryBuilder.hpp"

#include <vector>

namespace ocr::context {

class TableCompactor {
public:
    static LlmContextTable compactTable(
        const representation::SemanticTable& table,
        const LlmContextConfig& config,
        SemanticInventory& inventory
    );

    static std::vector<LlmContextTable> compactTables(
        const std::vector<representation::SemanticTable>& tables,
        const LlmContextConfig& config,
        SemanticInventory& inventory
    );
};

} // namespace ocr::context
