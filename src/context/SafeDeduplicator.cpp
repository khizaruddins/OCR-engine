#include "SafeDeduplicator.hpp"
#include "../representation/TextNormalizer.hpp"

#include <map>

namespace edmo::ocr::context {

DeduplicationResult SafeDeduplicator::deduplicate(
    const representation::SemanticDocument& doc,
    const DeduplicationConfig& config,
    SemanticInventory& inventory
) {
    DeduplicationResult res;
    if (!config.deduplicateHeaders && !config.deduplicateFooters && !config.deduplicatePageNumbers) {
        return res;
    }

    if (doc.pages.size() < config.minOccurrences) {
        return res;
    }

    // 1. Collect candidate header and footer texts across pages
    std::map<std::string, std::vector<std::string>> headerTextToBlockIds;
    std::map<std::string, std::vector<std::string>> footerTextToBlockIds;

    for (const auto& page : doc.pages) {
        int pageHeight = page.height > 0 ? page.height : 2200;
        int headerThresholdY = static_cast<int>(pageHeight * 0.18); // top 18%
        int footerThresholdY = static_cast<int>(pageHeight * 0.88); // bottom 12%

        for (const auto& block : page.blocks) {
            std::string normText = representation::TextNormalizer::collapseWhitespace(
                representation::TextNormalizer::trim(block.text)
            );
            if (normText.empty() || normText.size() < 4) continue;

            // Never deduplicate table content or label-values that look like individual student data
            if (block.type == representation::BlockType::LabelValue) {
                std::string upper = representation::TextNormalizer::toUpper(block.label);
                if (upper == "NAME" || upper == "STUDENT ID" || upper == "ROLL NO" || upper == "HTNO" || upper == "HALL TICKET") {
                    continue;
                }
            }

            int blockY = block.source.bbox.y;
            if (config.deduplicateHeaders && (blockY <= headerThresholdY || block.type == representation::BlockType::Header)) {
                headerTextToBlockIds[normText].push_back(block.id);
            } else if (config.deduplicateFooters && (blockY >= footerThresholdY || block.type == representation::BlockType::Footer)) {
                footerTextToBlockIds[normText].push_back(block.id);
            }
        }
    }

    // 2. Identify repeating headers
    std::string combinedHeader;
    for (const auto& [text, blockIds] : headerTextToBlockIds) {
        if (blockIds.size() >= config.minOccurrences) {
            if (!combinedHeader.empty()) combinedHeader += " | ";
            combinedHeader += text;
            for (const auto& id : blockIds) {
                res.deduplicatedBlockIds.insert(id);
                inventory.recordFate(id, ItemFate::Deduplicated, "Repeated document header banner across " + std::to_string(blockIds.size()) + " pages");
            }
        }
    }
    res.documentHeader = std::move(combinedHeader);

    // 3. Identify repeating footers
    std::string combinedFooter;
    for (const auto& [text, blockIds] : footerTextToBlockIds) {
        if (blockIds.size() >= config.minOccurrences) {
            if (!combinedFooter.empty()) combinedFooter += " | ";
            combinedFooter += text;
            for (const auto& id : blockIds) {
                res.deduplicatedBlockIds.insert(id);
                inventory.recordFate(id, ItemFate::Deduplicated, "Repeated document footer across " + std::to_string(blockIds.size()) + " pages");
            }
        }
    }
    res.documentFooter = std::move(combinedFooter);

    return res;
}

} // namespace edmo::ocr::context
