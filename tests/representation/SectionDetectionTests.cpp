#include <gtest/gtest.h>
#include "../../src/representation/SectionDetector.hpp"
#include "../../src/representation/BlockGrouper.hpp"

using namespace edmo::ocr;
using namespace edmo::ocr::representation;

TEST(SectionDetectionTests, SectionClassification) {
    EXPECT_EQ(SectionDetector::classifySectionFromText("STUDENT INFORMATION"), SectionType::StudentInformation);
    EXPECT_EQ(SectionDetector::classifySectionFromText("NAME : Sophia Jones"), SectionType::StudentInformation);
    EXPECT_EQ(SectionDetector::classifySectionFromText("BOSTON COLLEGE"), SectionType::InstitutionInformation);
    EXPECT_EQ(SectionDetector::classifySectionFromText("DEGREE : BACHELOR OF SCIENCE"), SectionType::AcademicInformation);
    EXPECT_EQ(SectionDetector::classifySectionFromText("SPRING 2024 MORRISSEY COLLEGE"), SectionType::CourseHistory);
    EXPECT_EQ(SectionDetector::classifySectionFromText("TOTAL CREDITS EARNED: 121 CUMGPA: 3.437"), SectionType::AcademicSummary);
}

TEST(SectionDetectionTests, ExtractGlobalEntities) {
    std::vector<OcrLine> lines = {
        {"BOSTON COLLEGE", 0.98f, {100, 100, 400, 40}, {}},
        {"NAME : Sophia Jones", 0.96f, {100, 300, 400, 30}, {}},
        {"STUDENT ID > 2291829", 0.95f, {100, 340, 300, 30}, {}},
        {"SCHOOL : MORRISSEY COLLEGE OF ARTS AND SCIENCES", 0.94f, {100, 400, 600, 30}, {}},
        {"DEGREE : BACHELOR OF SCIENCE - 05/19/2025", 0.95f, {100, 440, 600, 30}, {}},
        {"MAJOR : BIOLOGY", 0.93f, {100, 480, 300, 30}, {}}
    };

    RepresentationConfig config;
    auto blocks = BlockGrouper::groupLines(lines, 1, 1000, 2000, config);

    std::vector<SemanticTable> tables;
    auto sections = SectionDetector::detectSections(blocks, tables, config.sections);

    std::map<std::string, std::string> identity;
    std::map<std::string, std::string> institution;
    std::map<std::string, std::string> academic;

    SectionDetector::extractGlobalEntities(sections, identity, institution, academic);

    EXPECT_EQ(identity["student_name"], "Sophia Jones");
    EXPECT_EQ(identity["student_id"], "2291829");
    EXPECT_EQ(institution["name"], "Boston College");
    EXPECT_EQ(academic["degree"], "BACHELOR OF SCIENCE - 05/19/2025");
    EXPECT_EQ(academic["major"], "BIOLOGY");
}

TEST(SectionDetectionTests, CompositeAcademicLabelsRecognizedAsMajor) {
    RepresentationConfig config;
    std::vector<SemanticTable> tables;

    std::vector<std::string> compositeLabels = {
        "MAJOR",
        "BACHELORS MAJOR",
        "BACHELOR'S MAJOR",
        "UNDERGRADUATE MAJOR",
        "INTENDED MAJOR",
        "PROGRAM/MAJOR",
        "PROGRAM / MAJOR"
    };

    for (const auto& label : compositeLabels) {
        std::vector<OcrLine> lines = {
            {label + " : Accounting & Finance", 0.95f, {100, 300, 500, 30}, {}}
        };
        auto blocks = BlockGrouper::groupLines(lines, 1, 1000, 2000, config);
        auto sections = SectionDetector::detectSections(blocks, tables, config.sections);

        std::map<std::string, std::string> identity;
        std::map<std::string, std::string> institution;
        std::map<std::string, std::string> academic;
        SectionDetector::extractGlobalEntities(sections, identity, institution, academic);

        EXPECT_EQ(academic["major"], "Accounting & Finance")
            << "Failed to recognize composite academic label: " << label;
    }
}

