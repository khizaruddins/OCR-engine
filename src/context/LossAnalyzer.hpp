#pragma once

#include <ocr/context/LlmContextConfig.hpp>
#include <ocr/context/LlmContextModels.hpp>
#include <ocr/context/LlmContextReport.hpp>
#include <ocr/representation/SemanticDocument.hpp>
#include "SemanticInventoryBuilder.hpp"

namespace ocr::context {

class LossAnalyzer {
public:
    static LlmContextReport analyze(
        const representation::SemanticDocument& doc,
        const LlmContext& context,
        const SemanticInventory& inventory,
        const nlohmann::json* optionalRawOcrJson = nullptr
    );
};

} // namespace ocr::context
