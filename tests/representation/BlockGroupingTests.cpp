#include <gtest/gtest.h>
#include "../../src/representation/BlockGrouper.hpp"

using namespace ocr;
using namespace ocr::representation;

TEST(BlockGroupingTests, ParseLabelValue) {
    std::string label, value;

    EXPECT_TRUE(BlockGrouper::parseLabelValue("NAME : Sophia Jones", label, value));
    EXPECT_EQ(label, "NAME");
    EXPECT_EQ(value, "Sophia Jones");

    EXPECT_TRUE(BlockGrouper::parseLabelValue("STUDENT ID > 2291829", label, value));
    EXPECT_EQ(label, "STUDENT ID");
    EXPECT_EQ(value, "2291829");

    EXPECT_TRUE(BlockGrouper::parseLabelValue("DEGREE : BACHELOR OF SCIENCE - 05/19/2025", label, value));
    EXPECT_EQ(label, "DEGREE");
    EXPECT_EQ(value, "BACHELOR OF SCIENCE - 05/19/2025");

    EXPECT_TRUE(BlockGrouper::parseLabelValue("TERM GPA: 3.722", label, value));
    EXPECT_EQ(label, "TERM GPA");
    EXPECT_EQ(value, "3.722");

    EXPECT_FALSE(BlockGrouper::parseLabelValue("Just a regular narrative sentence without colons.", label, value));
}

TEST(BlockGroupingTests, HeadingDetection) {
    EXPECT_TRUE(BlockGrouper::isHeading("STUDENT INFORMATION"));
    EXPECT_TRUE(BlockGrouper::isHeading("ACADEMIC RECORD"));
    EXPECT_TRUE(BlockGrouper::isHeading("SPRING 2024 MORRISSEY COLLEGE OF ARTS AND SCIENCES"));
    EXPECT_TRUE(BlockGrouper::isHeading("FALL 2024"));
    EXPECT_TRUE(BlockGrouper::isHeading("TOTAL CREDITS EARNED"));
    EXPECT_FALSE(BlockGrouper::isHeading("This is a long descriptive paragraph about student academic standing and honors awarded."));
}

TEST(BlockGroupingTests, GroupLinesIntoBlocks) {
    std::vector<OcrLine> lines = {
        {"STUDENT INFORMATION", 0.95f, {100, 200, 300, 30}, {}},
        {"NAME : Sophia Jones", 0.96f, {100, 250, 400, 30}, {}},
        {"STUDENT ID > 2291829", 0.94f, {100, 290, 350, 30}, {}}
    };

    RepresentationConfig config;
    auto blocks = BlockGrouper::groupLines(lines, 1, 1000, 2000, config);

    ASSERT_EQ(blocks.size(), 3);
    EXPECT_EQ(blocks[0].type, BlockType::Heading);
    EXPECT_EQ(blocks[1].type, BlockType::LabelValue);
    EXPECT_EQ(blocks[1].label, "NAME");
    EXPECT_EQ(blocks[1].value, "Sophia Jones");
    EXPECT_EQ(blocks[2].type, BlockType::LabelValue);
    EXPECT_EQ(blocks[2].label, "STUDENT ID");
    EXPECT_EQ(blocks[2].value, "2291829");
}
