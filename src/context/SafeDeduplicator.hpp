#pragma once

#include <ocr/context/LlmContextConfig.hpp>
#include <ocr/context/LlmContextModels.hpp>
#include <ocr/representation/SemanticDocument.hpp>
#include "SemanticInventoryBuilder.hpp"

#include <set>
#include <string>

namespace ocr::context {

struct DeduplicationResult {
    std::string documentHeader;
    std::string documentFooter;
    std::set<std::string> deduplicatedBlockIds;
};

class SafeDeduplicator {
public:
    static DeduplicationResult deduplicate(
        const representation::SemanticDocument& doc,
        const DeduplicationConfig& config,
        SemanticInventory& inventory
    );
};

} // namespace ocr::context
