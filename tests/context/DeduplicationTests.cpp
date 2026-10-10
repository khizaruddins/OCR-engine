#include <gtest/gtest.h>
#include <ocr/representation/SemanticDocument.hpp>
#include "../../src/context/SafeDeduplicator.hpp"
#include "../../src/context/SemanticInventoryBuilder.hpp"

using namespace ocr;
using namespace ocr::context;
using namespace ocr::representation;

TEST(DeduplicationTests, SafelyDeduplicatesRepeatedHeaderWithoutLosingData) {
    SemanticDocument doc;
    doc.metadata.id = "multi_page_doc";
    doc.metadata.pageCount = 3;

    for (int p = 1; p <= 3; ++p) {
        SemanticPage page;
        page.pageNumber = p;
        page.height = 2200;

        RepresentationBlock header;
        header.id = "hdr_p" + std::to_string(p);
        header.text = "CVR COLLEGE OF ENGINEERING (Autonomous)";
        header.type = BlockType::Header;
        header.source.bbox = {100, 50, 800, 40};
        page.blocks.push_back(header);

        RepresentationBlock content;
        content.id = "content_p" + std::to_string(p);
        content.text = "Course Record for Semester " + std::to_string(p);
        content.type = BlockType::Paragraph;
        content.source.bbox = {100, 500, 600, 30};
        page.blocks.push_back(content);

        doc.pages.push_back(page);
    }

    auto inventory = SemanticInventoryBuilder::build(doc);
    DeduplicationConfig config;
    config.deduplicateHeaders = true;
    config.minOccurrences = 2;

    auto result = SafeDeduplicator::deduplicate(doc, config, inventory);

    EXPECT_FALSE(result.documentHeader.empty());
    EXPECT_TRUE(result.documentHeader.find("CVR COLLEGE OF ENGINEERING") != std::string::npos);
    EXPECT_EQ(result.deduplicatedBlockIds.size(), 3);

    // Verify header blocks were marked as Deduplicated
    for (int p = 1; p <= 3; ++p) {
        std::string hId = "hdr_p" + std::to_string(p);
        size_t idx = inventory.itemIndexById.at(hId);
        EXPECT_EQ(inventory.items[idx].fate, ItemFate::Deduplicated);
    }
}
