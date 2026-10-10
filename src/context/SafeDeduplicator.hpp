#pragma once

#include <edmo/ocr/context/LlmContextConfig.hpp>
#include <edmo/ocr/context/LlmContextModels.hpp>
#include <edmo/ocr/representation/SemanticDocument.hpp>
#include "SemanticInventoryBuilder.hpp"

#include <set>
#include <string>

namespace edmo::ocr::context {

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

} // namespace edmo::ocr::context
