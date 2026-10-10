#pragma once

#include <edmo/ocr/context/LlmContextConfig.hpp>
#include <edmo/ocr/context/LlmContextModels.hpp>
#include <edmo/ocr/representation/SemanticDocument.hpp>
#include "SafeDeduplicator.hpp"
#include "SemanticInventoryBuilder.hpp"

#include <vector>

namespace edmo::ocr::context {

struct TextCompactionResult {
    std::vector<LlmContextSection> sections;
    std::vector<std::string> unclassifiedContent;
};

class TextCompactor {
public:
    static TextCompactionResult compact(
        const representation::SemanticDocument& doc,
        const DeduplicationResult& dedupResult,
        const LlmContextConfig& config,
        SemanticInventory& inventory
    );
};

} // namespace edmo::ocr::context
