#pragma once

#include <edmo/ocr/context/LlmContextConfig.hpp>
#include <edmo/ocr/context/LlmContextModels.hpp>
#include <edmo/ocr/context/LlmContextReport.hpp>
#include <edmo/ocr/representation/SemanticDocument.hpp>
#include "SemanticInventoryBuilder.hpp"

namespace edmo::ocr::context {

class LossAnalyzer {
public:
    static LlmContextReport analyze(
        const representation::SemanticDocument& doc,
        const LlmContext& context,
        const SemanticInventory& inventory,
        const nlohmann::json* optionalRawOcrJson = nullptr
    );
};

} // namespace edmo::ocr::context
