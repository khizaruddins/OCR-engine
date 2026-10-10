#include <gtest/gtest.h>
#include <edmo/ocr/representation/DocumentRepresentationEngine.hpp>
#include <edmo/ocr/representation/EdrJsonSerializer.hpp>

using namespace edmo::ocr::representation;

TEST(EdrSerializationTests, SchemaCompliance) {
    SemanticDocument doc;
    doc.schemaVersion = "edr-0.1";
    doc.documentType = "transcript";
    doc.metadata.id = "test_doc";
    doc.metadata.pageCount = 1;

    doc.identity["student_name"] = "Sophia Jones";
    doc.identity["student_id"] = "123456";

    doc.institution["name"] = "Example University";

    doc.academic["degree"] = "Bachelor of Science";
    doc.academic["major"] = "Computer Science";

    SemanticTable tbl;
    tbl.id = "table_1";
    tbl.tableType = "course_records";
    tbl.columns = {
        {"course_code", "course_code", 0, 0, 0, 0.95f, {}},
        {"course_title", "course_title", 1, 0, 0, 0.95f, {}},
        {"credits", "credits", 2, 0, 0, 0.95f, {}},
        {"grade", "grade", 3, 0, 0, 0.95f, {}}
    };

    SemanticRow row1;
    row1.canonicalFields["course_code"] = "CS101";
    row1.canonicalFields["course_title"] = "Introduction to Computer Science";
    row1.canonicalFields["credits"] = "3";
    row1.canonicalFields["grade"] = "A";
    tbl.rows.push_back(row1);

    doc.tables.push_back(tbl);

    // 1. Full JSON
    nlohmann::json fullJson = EdrJsonSerializer::toFullJson(doc);
    EXPECT_EQ(fullJson["schema_version"], "edr-0.1");
    EXPECT_EQ(fullJson["document"]["type"], "transcript");
    EXPECT_EQ(fullJson["identity"]["student_name"], "Sophia Jones");
    EXPECT_EQ(fullJson["institution"]["name"], "Example University");
    EXPECT_EQ(fullJson["academic"]["degree"], "Bachelor of Science");
    EXPECT_EQ(fullJson["tables"].size(), 1);

    // 2. LLM JSON
    nlohmann::json llmJson = EdrJsonSerializer::toLlmJson(doc, false);
    EXPECT_EQ(llmJson["document_type"], "transcript");
    EXPECT_EQ(llmJson["student"]["student_name"], "Sophia Jones");
    EXPECT_EQ(llmJson["institution"]["name"], "Example University");
    EXPECT_EQ(llmJson["academic"]["degree"], "Bachelor of Science");
    EXPECT_EQ(llmJson["academic_records"].size(), 1);
    EXPECT_EQ(llmJson["academic_records"][0]["course_code"], "CS101");
    EXPECT_EQ(llmJson["academic_records"][0]["course_title"], "Introduction to Computer Science");
    EXPECT_EQ(llmJson["academic_records"][0]["credits"], "3");
    EXPECT_EQ(llmJson["academic_records"][0]["grade"], "A");
}
