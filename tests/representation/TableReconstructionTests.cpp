#include <gtest/gtest.h>
#include "../../src/representation/TableReconstructor.hpp"

using namespace edmo::ocr;
using namespace edmo::ocr::representation;

TEST(TableReconstructionTests, ParseCourseRowSophiaStyle) {
    OcrLine line;
    line.bbox = {100, 500, 600, 30};
    line.confidence = 0.95f;
    line.tokens = {
        {"CS101", 0.96f, {100, 500, 70, 30}},
        {"Introduction", 0.94f, {200, 500, 120, 30}},
        {"to", 0.95f, {330, 500, 30, 30}},
        {"Computer", 0.95f, {370, 500, 90, 30}},
        {"Science", 0.95f, {470, 500, 80, 30}},
        {"3", 0.93f, {580, 500, 20, 30}},
        {"A", 0.97f, {650, 500, 20, 30}}
    };

    TableConfig config;
    SemanticRow row = TableReconstructor::parseCourseRow(line, 1, config);

    EXPECT_EQ(row.canonicalFields["course_code"], "CS101");
    EXPECT_EQ(row.canonicalFields["course_title"], "Introduction to Computer Science");
    EXPECT_EQ(row.canonicalFields["credits"], "3");
    EXPECT_EQ(row.canonicalFields["grade"], "A");
    EXPECT_EQ(row.source.pageNumber, 1);
}

TEST(TableReconstructionTests, ParseCourseRowBostonStyleWithAttemptedAndEarned) {
    OcrLine line;
    line.bbox = {100, 500, 800, 30};
    line.confidence = 0.94f;
    line.tokens = {
        {"BIOL4570", 0.95f, {100, 500, 90, 30}},
        {"PRINCIPLES", 0.95f, {210, 500, 110, 30}},
        {"OF", 0.95f, {330, 500, 30, 30}},
        {"IMMUNOLOGY", 0.95f, {370, 500, 140, 30}},
        {"03", 0.92f, {550, 500, 30, 30}},
        {"03", 0.92f, {620, 500, 30, 30}},
        {"C+", 0.94f, {700, 500, 30, 30}}
    };

    TableConfig config;
    SemanticRow row = TableReconstructor::parseCourseRow(line, 2, config);

    EXPECT_EQ(row.canonicalFields["course_code"], "BIOL4570");
    EXPECT_EQ(row.canonicalFields["course_title"], "PRINCIPLES OF IMMUNOLOGY");
    EXPECT_EQ(row.canonicalFields["attempted_credits"], "03");
    EXPECT_EQ(row.canonicalFields["earned_credits"], "03");
    EXPECT_EQ(row.canonicalFields["credits"], "03");
    EXPECT_EQ(row.canonicalFields["grade"], "C+");
    EXPECT_EQ(row.source.pageNumber, 2);
}

TEST(TableReconstructionTests, ParseCourseRowWithOcrNumberConfusion) {
    // UNAS1016 DESIGNING YOUR CAREER o1 o1 P
    OcrLine line;
    line.bbox = {100, 500, 800, 30};
    line.confidence = 0.90f;
    line.tokens = {
        {"UNAS1016", 0.92f, {100, 500, 90, 30}},
        {"DESIGNING", 0.92f, {210, 500, 100, 30}},
        {"YOUR", 0.92f, {320, 500, 60, 30}},
        {"CAREER", 0.92f, {390, 500, 80, 30}},
        {"o1", 0.88f, {550, 500, 30, 30}},
        {"o1", 0.88f, {620, 500, 30, 30}},
        {"P", 0.95f, {700, 500, 20, 30}}
    };

    TableConfig config;
    SemanticRow row = TableReconstructor::parseCourseRow(line, 2, config);

    EXPECT_EQ(row.canonicalFields["course_code"], "UNAS1016");
    EXPECT_EQ(row.canonicalFields["course_title"], "DESIGNING YOUR CAREER");
    EXPECT_EQ(row.canonicalFields["attempted_credits"], "01");
    EXPECT_EQ(row.canonicalFields["earned_credits"], "01");
    EXPECT_EQ(row.canonicalFields["grade"], "P");
}
