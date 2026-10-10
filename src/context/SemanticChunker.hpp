#pragma once

#include <ocr/context/LlmContextConfig.hpp>
#include <ocr/context/LlmContextModels.hpp>
#include "SemanticInventoryBuilder.hpp"

namespace ocr::context {

class SemanticChunker {
public:
    static void applyBudgetAndChunking(
        LlmContext& context,
        const LlmContextConfig& config,
        SemanticInventory& inventory
    );

    static size_t estimateJsonTokens(const nlohmann::json& j);
};

} // namespace ocr::context
