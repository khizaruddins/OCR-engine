#pragma once

#include <edmo/ocr/context/LlmContextConfig.hpp>
#include <edmo/ocr/context/LlmContextModels.hpp>
#include <edmo/ocr/representation/SemanticTable.hpp>
#include "SemanticInventoryBuilder.hpp"

#include <vector>

namespace edmo::ocr::context {

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

} // namespace edmo::ocr::context
