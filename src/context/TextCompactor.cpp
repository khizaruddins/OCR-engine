#include "TextCompactor.hpp"
#include "../representation/TextNormalizer.hpp"

#include <set>

namespace ocr::context {

TextCompactionResult TextCompactor::compact(
    const representation::SemanticDocument& doc,
    const DeduplicationResult& dedupResult,
    const LlmContextConfig& config,
    SemanticInventory& inventory
) {
    (void)config;
    TextCompactionResult res;

    // 1. Process Sections from SemanticDocument
    for (const auto& sec : doc.sections) {
        LlmContextSection outSec;
        outSec.id = sec.id;
        outSec.type = std::string(representation::sectionTypeToString(sec.type));
        outSec.title = sec.title;
        outSec.pageStart = sec.pageStart;
        outSec.pageEnd = sec.pageEnd;

        // Copy extracted fields
        for (const auto& [k, v] : sec.fields) {
            outSec.fields[k] = v;
        }

        // Process blocks within section
        for (const auto& block : sec.blocks) {
            if (dedupResult.deduplicatedBlockIds.count(block.id) > 0) {
                continue; // Already deduplicated
            }

            std::string cleanText = representation::TextNormalizer::collapseWhitespace(
                representation::TextNormalizer::trim(block.text)
            );
            if (cleanText.empty()) {
                inventory.recordFate(block.id, ItemFate::Noise, "Empty whitespace block");
                continue;
            }

            if (block.type == representation::BlockType::LabelValue && !block.label.empty()) {
                outSec.fields[block.label] = block.value;
                inventory.recordFate(block.id, ItemFate::Structured, "Structured as key-value field");
            } else if (block.type == representation::BlockType::Heading) {
                if (outSec.title.empty()) {
                    outSec.title = cleanText;
                } else {
                    outSec.paragraphs.push_back(cleanText);
                }
                inventory.recordFate(block.id, ItemFate::Structured, "Structured as section heading");
            } else {
                outSec.paragraphs.push_back(cleanText);
                inventory.recordFate(block.id, ItemFate::Compacted, "Compacted as section paragraph text");
            }
        }

        if (!outSec.fields.empty() || !outSec.paragraphs.empty() || !outSec.unclassified.empty()) {
            res.sections.push_back(std::move(outSec));
        }
    }

    // 2. Ensure all page blocks not captured in sections are accounted for
    std::set<std::string> capturedBlockIds = dedupResult.deduplicatedBlockIds;
    for (const auto& sec : doc.sections) {
        for (const auto& b : sec.blocks) {
            capturedBlockIds.insert(b.id);
        }
    }

    for (const auto& page : doc.pages) {
        LlmContextSection unclassifiedPageSec;
        unclassifiedPageSec.id = "section_page_" + std::to_string(page.pageNumber) + "_unclassified";
        unclassifiedPageSec.type = "unclassified";
        unclassifiedPageSec.title = "Page " + std::to_string(page.pageNumber) + " General Content";
        unclassifiedPageSec.pageStart = page.pageNumber;
        unclassifiedPageSec.pageEnd = page.pageNumber;

        for (const auto& block : page.blocks) {
            if (capturedBlockIds.count(block.id) > 0) continue;

            std::string cleanText = representation::TextNormalizer::collapseWhitespace(
                representation::TextNormalizer::trim(block.text)
            );
            if (cleanText.empty()) {
                inventory.recordFate(block.id, ItemFate::Noise, "Empty whitespace block");
                continue;
            }

            if (block.type == representation::BlockType::LabelValue && !block.label.empty()) {
                unclassifiedPageSec.fields[block.label] = block.value;
                inventory.recordFate(block.id, ItemFate::Structured, "Structured page unclassified field");
            } else {
                unclassifiedPageSec.paragraphs.push_back(cleanText);
                inventory.recordFate(block.id, ItemFate::Compacted, "Compacted page unclassified text");
            }
        }

        if (!unclassifiedPageSec.fields.empty() || !unclassifiedPageSec.paragraphs.empty()) {
            res.sections.push_back(std::move(unclassifiedPageSec));
        }
    }

    return res;
}

} // namespace ocr::context
