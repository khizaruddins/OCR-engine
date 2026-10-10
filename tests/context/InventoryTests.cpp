#include <gtest/gtest.h>
#include <ocr/representation/SemanticDocument.hpp>
#include "../../src/context/SemanticInventoryBuilder.hpp"

using namespace ocr;
using namespace ocr::context;
using namespace ocr::representation;

TEST(InventoryTests, CatalogsBlocksAndAssignsFates) {
    SemanticDocument doc;
    doc.metadata.id = "test_doc";
    doc.metadata.pageCount = 2;

    SemanticPage p1;
    p1.pageNumber = 1;
    RepresentationBlock b1;
    b1.id = "b1";
    b1.text = "University Header";
    b1.type = BlockType::Header;
    b1.confidence.mean = 0.95f;
    p1.blocks.push_back(b1);

    RepresentationBlock b2;
    b2.id = "b2";
    b2.text = "Student Name: Alice";
    b2.type = BlockType::LabelValue;
    b2.label = "Student Name";
    b2.value = "Alice";
    b2.confidence.mean = 0.55f; // Low confidence
    b2.confidence.lowConfidenceTokenCount = 1;
    p1.blocks.push_back(b2);

    doc.pages.push_back(p1);

    auto inventory = SemanticInventoryBuilder::build(doc);

    EXPECT_EQ(inventory.pageCount, 2);
    EXPECT_EQ(inventory.totalBlocks, 2);
    EXPECT_EQ(inventory.totalLowConfidenceItems, 1);
    EXPECT_GE(inventory.totalMeaningfulCharacters, 20);

    // Verify initial fate is Unaccounted
    EXPECT_EQ(inventory.items[0].fate, ItemFate::Unaccounted);

    // Record fate
    inventory.recordFate("b1", ItemFate::Structured, "Header structured");
    EXPECT_EQ(inventory.items[0].fate, ItemFate::Structured);
}
