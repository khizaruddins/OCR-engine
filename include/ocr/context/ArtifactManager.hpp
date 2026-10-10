#pragma once

#include <ocr/OcrResult.hpp>
#include <ocr/context/ArtifactPaths.hpp>
#include <ocr/context/LlmContextModels.hpp>
#include <ocr/context/LlmContextReport.hpp>
#include <ocr/representation/SemanticDocument.hpp>
#include <ocr/representation/EdrJsonSerializer.hpp>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace ocr::context {

struct WrittenArtifacts {
    std::filesystem::path documentDirectory;
    std::filesystem::path rawOcrPath;
    std::filesystem::path edrFullPath;
    std::filesystem::path llmContextPath;
    std::filesystem::path llmContextReportPath;
    bool success = true;
};

class ArtifactManager {
public:
    /**
     * @brief Write all 4 standard canonical artifacts to output/documents/<document-name>/
     */
    static WrittenArtifacts writeAll(
        const std::filesystem::path& baseOutputDir,
        std::string_view documentFilename,
        const OcrResult* optionalOcrResult,
        const representation::SemanticDocument& edrDoc,
        const LlmContext& llmContext,
        const LlmContextReport& llmReport,
        bool prettyPrint = true
    ) {
        WrittenArtifacts out;
        out.documentDirectory = ArtifactPaths::getDocumentDirectory(baseOutputDir, documentFilename);

        std::error_code ec;
        std::filesystem::create_directories(out.documentDirectory, ec);

        out.rawOcrPath = ArtifactPaths::rawOcrPath(out.documentDirectory);
        out.edrFullPath = ArtifactPaths::edrFullPath(out.documentDirectory);
        out.llmContextPath = ArtifactPaths::llmContextPath(out.documentDirectory);
        out.llmContextReportPath = ArtifactPaths::llmContextReportPath(out.documentDirectory);

        // 1. Raw OCR JSON
        if (optionalOcrResult != nullptr) {
            nlohmann::json rawJson = optionalOcrResult->toJson(OutputFormat::FullJson);
            if (!representation::EdrJsonSerializer::saveToFile(out.rawOcrPath, rawJson, prettyPrint)) {
                out.success = false;
            }
        }

        // 2. Full EDR JSON
        nlohmann::json fullEdrJson = representation::EdrJsonSerializer::toFullJson(edrDoc);
        if (!representation::EdrJsonSerializer::saveToFile(out.edrFullPath, fullEdrJson, prettyPrint)) {
            out.success = false;
        }

        // 3. LLM Context JSON
        nlohmann::json ctxJson = llmContext.toJson(false);
        if (!representation::EdrJsonSerializer::saveToFile(out.llmContextPath, ctxJson, prettyPrint)) {
            out.success = false;
        }

        // 4. LLM Context Report JSON
        nlohmann::json repJson = llmReport.toJson();
        if (!representation::EdrJsonSerializer::saveToFile(out.llmContextReportPath, repJson, prettyPrint)) {
            out.success = false;
        }

        return out;
    }
};

} // namespace ocr::context
