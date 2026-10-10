#pragma once

#include <ocr/context/LlmContextConfig.hpp>
#include <ocr/context/LlmContextModels.hpp>
#include <ocr/representation/SemanticDocument.hpp>
#include "SafeDeduplicator.hpp"
#include "SemanticInventoryBuilder.hpp"

#include <vector>

namespace ocr::context {

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

} // namespace ocr::context
