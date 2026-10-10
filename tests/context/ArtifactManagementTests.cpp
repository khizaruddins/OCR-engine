#include <gtest/gtest.h>
#include <edmo/ocr/context/ArtifactManager.hpp>
#include <edmo/ocr/context/ArtifactPaths.hpp>
#include <edmo/ocr/representation/SemanticDocument.hpp>

#include <filesystem>
#include <set>
#include <string>

using namespace edmo::ocr;
using namespace edmo::ocr::context;
using namespace edmo::ocr::representation;

TEST(ArtifactManagementTests, SanitizesComplexFilenamesCorrectly) {
    EXPECT_EQ(ArtifactPaths::sanitizeDocumentName("International transcript-IND_Redacted compressed.pdf"),
              "International_transcript-IND_Redacted_compressed");

    EXPECT_EQ(ArtifactPaths::sanitizeDocumentName("041077871_West Coast University.pdf"),
              "041077871_West_Coast_University");

    EXPECT_EQ(ArtifactPaths::sanitizeDocumentName("Sophia Jones Transcript.pdf"),
              "Sophia_Jones_Transcript");

    // Strips legacy suffixes so we never get double suffixes
    EXPECT_EQ(ArtifactPaths::sanitizeDocumentName("International_transcript_raw_ocr.json"),
              "International_transcript");

    EXPECT_EQ(ArtifactPaths::sanitizeDocumentName("Boston_transcript_edr_full.json"),
              "Boston_transcript");

    EXPECT_EQ(ArtifactPaths::sanitizeDocumentName("sample_doc_llm_context_report.json"),
              "sample_doc");
}

TEST(ArtifactManagementTests, JsonFilesAreNotTreatedAsDocuments) {
    EXPECT_FALSE(ArtifactPaths::isSupportedDocumentExtension(".json"));
    EXPECT_FALSE(ArtifactPaths::isSupportedDocumentExtension("json"));
    EXPECT_TRUE(ArtifactPaths::isSupportedDocumentExtension(".pdf"));
    EXPECT_TRUE(ArtifactPaths::isSupportedDocumentExtension(".png"));
    EXPECT_TRUE(ArtifactPaths::isSupportedDocumentExtension(".jpg"));
    EXPECT_TRUE(ArtifactPaths::isSupportedDocumentExtension(".jpeg"));
    EXPECT_TRUE(ArtifactPaths::isSupportedDocumentExtension(".tiff"));
    EXPECT_TRUE(ArtifactPaths::isSupportedDocumentExtension(".tif"));
}

TEST(ArtifactManagementTests, SingleDocumentProducesExactlyFourCanonicalArtifacts) {
    std::filesystem::path testBaseDir = std::filesystem::temp_directory_path() / "edmo_artifact_test_single";
    std::error_code ec;
    std::filesystem::remove_all(testBaseDir, ec);

    OcrResult ocr;
    SemanticDocument doc;
    doc.metadata.id = "doc_test_1";
    doc.metadata.pageCount = 1;

    LlmContext ctx;
    ctx.pagesPresent = {1};

    LlmContextReport rep;
    rep.documentId = "doc_test_1";

    auto written = ArtifactManager::writeAll(
        testBaseDir,
        "International transcript-IND_Redacted compressed.pdf",
        &ocr,
        doc,
        ctx,
        rep,
        true
    );

    ASSERT_TRUE(written.success);
    EXPECT_EQ(written.documentDirectory.filename().string(), "International_transcript-IND_Redacted_compressed");

    // Count files in the document directory
    std::set<std::string> actualFiles;
    for (const auto& entry : std::filesystem::directory_iterator(written.documentDirectory, ec)) {
        if (entry.is_regular_file()) {
            actualFiles.insert(entry.path().filename().string());
        }
    }

    std::set<std::string> expectedFiles = {
        "raw_ocr.json",
        "edr_full.json",
        "llm_context.json",
        "llm_context_report.json"
    };

    EXPECT_EQ(actualFiles, expectedFiles);

    // Verify no recursive / double suffixes exist
    for (const auto& f : actualFiles) {
        EXPECT_EQ(f.find("_raw_ocr_edr_full"), std::string::npos);
        EXPECT_EQ(f.find("_edr_full_edr_full"), std::string::npos);
        EXPECT_EQ(f.find("_raw_ocr_llm_context"), std::string::npos);
    }

    std::filesystem::remove_all(testBaseDir, ec);
}

TEST(ArtifactManagementTests, ReRunOverwritesCleanlyWithoutDuplicates) {
    std::filesystem::path testBaseDir = std::filesystem::temp_directory_path() / "edmo_artifact_test_rerun";
    std::error_code ec;
    std::filesystem::remove_all(testBaseDir, ec);

    OcrResult ocr;
    SemanticDocument doc;
    doc.metadata.id = "doc_rerun";
    doc.metadata.pageCount = 1;

    LlmContext ctx;
    ctx.pagesPresent = {1};

    LlmContextReport rep;

    // Run 1
    ArtifactManager::writeAll(testBaseDir, "transcript.pdf", &ocr, doc, ctx, rep);
    // Run 2 (re-run)
    ArtifactManager::writeAll(testBaseDir, "transcript.pdf", &ocr, doc, ctx, rep);

    auto docDir = testBaseDir / "transcript";
    size_t fileCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(docDir, ec)) {
        if (entry.is_regular_file()) fileCount++;
    }

    EXPECT_EQ(fileCount, 4); // Exactly 4 files, no _1, _2 or duplicates created

    std::filesystem::remove_all(testBaseDir, ec);
}

TEST(ArtifactManagementTests, MultipleDocumentsAreIsolatedInSeparateDirectories) {
    std::filesystem::path testBaseDir = std::filesystem::temp_directory_path() / "edmo_artifact_test_multi";
    std::error_code ec;
    std::filesystem::remove_all(testBaseDir, ec);

    SemanticDocument doc;
    LlmContext ctx;
    LlmContextReport rep;

    ArtifactManager::writeAll(testBaseDir, "Sophia Jones Transcript.pdf", nullptr, doc, ctx, rep);
    ArtifactManager::writeAll(testBaseDir, "Boston Transcript.pdf", nullptr, doc, ctx, rep);

    EXPECT_TRUE(std::filesystem::exists(testBaseDir / "Sophia_Jones_Transcript" / "llm_context.json"));
    EXPECT_TRUE(std::filesystem::exists(testBaseDir / "Boston_Transcript" / "llm_context.json"));

    std::filesystem::remove_all(testBaseDir, ec);
}
