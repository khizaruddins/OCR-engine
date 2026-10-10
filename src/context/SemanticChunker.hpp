#pragma once

#include <edmo/ocr/context/LlmContextConfig.hpp>
#include <edmo/ocr/context/LlmContextModels.hpp>
#include "SemanticInventoryBuilder.hpp"

namespace edmo::ocr::context {

class SemanticChunker {
public:
    static void applyBudgetAndChunking(
        LlmContext& context,
        const LlmContextConfig& config,
        SemanticInventory& inventory
    );

    static size_t estimateJsonTokens(const nlohmann::json& j);
};

} // namespace edmo::ocr::context
