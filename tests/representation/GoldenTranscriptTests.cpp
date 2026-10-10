#include <gtest/gtest.h>
#include <edmo/ocr/representation/DocumentRepresentationEngine.hpp>
#include <edmo/ocr/representation/EdrJsonSerializer.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace edmo::ocr::representation;

TEST(GoldenTranscriptTests, ProcessBostonTranscriptRawJson) {
    std::vector<std::filesystem::path> candidatePaths = {
        "output/documents/boston_transcript/raw_ocr.json",
        "../output/documents/boston_transcript/raw_ocr.json",
        "/home/khizaruddin/projects/fullapp/EDMO-OCR-DI/EDMO C++ OCR Engine/OCR Layer/output/documents/boston_transcript/raw_ocr.json",
        "output/raw_json/boston_transcript.json",
        "../output/raw_json/boston_transcript.json",
        "/home/khizaruddin/projects/fullapp/EDMO-OCR-DI/EDMO C++ OCR Engine/OCR Layer/output/raw_json/boston_transcript.json"
    };

    std::filesystem::path bostonPath;
    for (const auto& p : candidatePaths) {
        if (std::filesystem::exists(p)) {
            bostonPath = p;
            break;
        }
    }

    if (bostonPath.empty()) {
        GTEST_SKIP() << "boston_transcript.json not found in candidate paths";
    }

    DocumentRepresentationEngine engine;
    auto res = engine.transform(bostonPath);
    ASSERT_TRUE(res.has_value()) << res.error().message;

    const auto& doc = res.value();

    EXPECT_EQ(doc.documentType, "transcript");
    EXPECT_EQ(doc.metadata.pageCount, 2);

    // Verify Institution
    EXPECT_TRUE(doc.institution.count("name") > 0);
    EXPECT_EQ(doc.institution.at("name"), "Boston College");

    // Verify Academic fields
    EXPECT_TRUE(doc.academic.count("degree") > 0);
    EXPECT_NE(doc.academic.at("degree").find("BACHELOR OF SCIENCE"), std::string::npos);

    EXPECT_TRUE(doc.academic.count("major") > 0);
    EXPECT_EQ(doc.academic.at("major"), "BIOLOGY");

    // Verify Course tables and rows
    EXPECT_GE(doc.tables.size(), 1);

    size_t totalCourseRows = 0;
    std::map<std::string, std::string> foundCourses;

    for (const auto& tbl : doc.tables) {
        totalCourseRows += tbl.rows.size();
        for (const auto& row : tbl.rows) {
            if (row.canonicalFields.count("course_code")) {
                std::string code = row.canonicalFields.at("course_code");
                std::string title = row.canonicalFields.count("course_title") ? row.canonicalFields.at("course_title") : "";
                foundCourses[code] = title;
            }
        }
    }

    EXPECT_GE(totalCourseRows, 10);

    // Verify key courses survived
    EXPECT_TRUE(foundCourses.count("BIOL4570") > 0);
    EXPECT_TRUE(foundCourses.count("BIOL4960") > 0);
    EXPECT_TRUE(foundCourses.count("CSCI1101") > 0);
    EXPECT_TRUE(foundCourses.count("EALC1122") > 0);
    EXPECT_TRUE(foundCourses.count("PHCG1210") > 0);
    EXPECT_TRUE(foundCourses.count("CHEM4461") > 0);
    EXPECT_TRUE(foundCourses.count("THEO1431") > 0);
    EXPECT_TRUE(foundCourses.count("UNAS1016") > 0);
    EXPECT_TRUE(foundCourses.count("BIOL4030") > 0);

    // Verify Compression Performance
    EXPECT_GT(doc.compression.rawBytes, 50000); // Boston raw minified is ~108 KB (276 KB on disk)
    EXPECT_GT(doc.compression.rawToLlmRatio, 8.0); // LLM EDR achieves huge reduction (>15x)
    EXPECT_GE(doc.compression.textPreservationPercentage, 90.0);

    // Verify LLM Serialization
    nlohmann::json llmJson = EdrJsonSerializer::toLlmJson(doc, false);
    EXPECT_EQ(llmJson["document_type"], "transcript");
    EXPECT_TRUE(llmJson.contains("academic_records"));
    EXPECT_GE(llmJson["academic_records"].size(), 10);
}
