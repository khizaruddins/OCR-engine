#include <gtest/gtest.h>
#include "../../src/representation/TableDetector.hpp"

using namespace ocr;
using namespace ocr::representation;

TEST(TableDetectionTests, CourseCodePatterns) {
    EXPECT_TRUE(TableDetector::isCourseCode("CS101"));
    EXPECT_TRUE(TableDetector::isCourseCode("CS102"));
    EXPECT_TRUE(TableDetector::isCourseCode("CS-101"));
    EXPECT_TRUE(TableDetector::isCourseCode("MATH101"));
    EXPECT_TRUE(TableDetector::isCourseCode("BIOL4570"));
    EXPECT_TRUE(TableDetector::isCourseCode("CSCI1101"));
    EXPECT_TRUE(TableDetector::isCourseCode("PHCG1210"));
    EXPECT_TRUE(TableDetector::isCourseCode("UNAS1016"));
    EXPECT_TRUE(TableDetector::isCourseCode("EALC1122"));
    EXPECT_TRUE(TableDetector::isCourseCode("BIO101A"));

    EXPECT_FALSE(TableDetector::isCourseCode("UNIVERSITY"));
    EXPECT_FALSE(TableDetector::isCourseCode("BACHELOR"));
    EXPECT_FALSE(TableDetector::isCourseCode("123456"));
}

TEST(TableDetectionTests, GradeTokens) {
    TableConfig config;
    EXPECT_TRUE(TableDetector::isGradeToken("A", config.gradeVocabulary));
    EXPECT_TRUE(TableDetector::isGradeToken("A-", config.gradeVocabulary));
    EXPECT_TRUE(TableDetector::isGradeToken("B+", config.gradeVocabulary));
    EXPECT_TRUE(TableDetector::isGradeToken("C", config.gradeVocabulary));
    EXPECT_TRUE(TableDetector::isGradeToken("P", config.gradeVocabulary));
    EXPECT_TRUE(TableDetector::isGradeToken("W", config.gradeVocabulary));
    EXPECT_TRUE(TableDetector::isGradeToken("Ww", config.gradeVocabulary));

    EXPECT_FALSE(TableDetector::isGradeToken("COLLEGE", config.gradeVocabulary));
    EXPECT_FALSE(TableDetector::isGradeToken("SCIENCE", config.gradeVocabulary));
}

TEST(TableDetectionTests, NumericCreditTokens) {
    EXPECT_TRUE(TableDetector::isNumericCreditToken("3"));
    EXPECT_TRUE(TableDetector::isNumericCreditToken("03"));
    EXPECT_TRUE(TableDetector::isNumericCreditToken("3.0"));
    EXPECT_TRUE(TableDetector::isNumericCreditToken("4.0"));
    EXPECT_TRUE(TableDetector::isNumericCreditToken("o1"));
    EXPECT_TRUE(TableDetector::isNumericCreditToken("00"));

    EXPECT_FALSE(TableDetector::isNumericCreditToken("COURSE"));
    EXPECT_FALSE(TableDetector::isNumericCreditToken("GRADE"));
}

TEST(TableDetectionTests, TableHeaderDetection) {
    OcrLine line;
    line.text = "COURSE       COURSE TITLE                                      ATT EARN GR";
    EXPECT_TRUE(TableDetector::isTableHeaderLine(line));

    OcrLine regularLine;
    regularLine.text = "BACHELOR OF SCIENCE IN COMPUTER SCIENCE";
    EXPECT_FALSE(TableDetector::isTableHeaderLine(regularLine));
}
