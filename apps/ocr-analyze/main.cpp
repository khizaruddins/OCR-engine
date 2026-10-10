#include <ocr/OcrResult.hpp>
#include <ocr/context/ArtifactPaths.hpp>
#include <ocr/context/LlmContextBuilder.hpp>
#include "../../src/context/LlmContextSerializer.hpp"
#include <ocr/representation/DocumentRepresentationEngine.hpp>
#include <ocr/representation/EdrJsonSerializer.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

namespace {

void printHelp(const char* prog) {
    std::cout << "OCR-DI Document Representation & Context Analyzer v0.1.0\n\n"
              << "Usage:\n"
              << "  " << prog << " --raw <raw-ocr.json> [options]\n"
              << "  " << prog << " --raw <raw-ocr.json> --output-dir results/\n\n"
              << "Options:\n"
              << "  --raw <path>             Path to raw OCR JSON file (input)\n"
              << "  --edr-full <path>        Path to Full EDR JSON file (input/output)\n"
              << "  --edr-llm <path>         Path to Legacy LLM EDR JSON file (input/output)\n"
              << "  --llm-context <path>     Path to Loss-Aware LLM Context JSON file (output)\n"
              << "  --llm-report <path>      Path to LLM Context Report JSON file (output)\n"
              << "  --llm-token-budget <N>   Token budget for LLM Context Builder (default: 12000)\n"
              << "  -o, --output-dir <dir>   Output directory to write all generated artifacts\n"
              << "  -h, --help               Show this help message\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp(argv[0]);
        return 1;
    }

    std::string rawPath;
    std::string fullPath;
    std::string llmPath;
    std::string contextPath;
    std::string reportPath;
    std::string outDir;
    size_t tokenBudget = 12000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--raw" && i + 1 < argc) {
            rawPath = argv[++i];
        } else if (arg == "--edr-full" && i + 1 < argc) {
            fullPath = argv[++i];
        } else if (arg == "--edr-llm" && i + 1 < argc) {
            llmPath = argv[++i];
        } else if (arg == "--llm-context" && i + 1 < argc) {
            contextPath = argv[++i];
        } else if (arg == "--llm-report" && i + 1 < argc) {
            reportPath = argv[++i];
        } else if (arg == "--llm-token-budget" && i + 1 < argc) {
            tokenBudget = static_cast<size_t>(std::stoul(argv[++i]));
        } else if ((arg == "-o" || arg == "--output-dir") && i + 1 < argc) {
            outDir = argv[++i];
        } else if (arg[0] != '-' && rawPath.empty()) {
            rawPath = arg;
        }
    }

    if (rawPath.empty()) {
        std::cerr << "Error: --raw path must be specified.\n";
        return 1;
    }

    std::filesystem::path rawFilePath(rawPath);
    if (!std::filesystem::exists(rawFilePath)) {
        std::cerr << "Error: Raw OCR file not found: " << rawPath << "\n";
        return 1;
    }

    // 1. Read Raw OCR JSON
    std::ifstream rawFile(rawFilePath);
    if (!rawFile.is_open()) {
        std::cerr << "Error: Failed to open raw OCR file: " << rawPath << "\n";
        return 1;
    }

    nlohmann::json rawJson;
    try {
        rawFile >> rawJson;
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to parse raw OCR JSON: " << e.what() << "\n";
        return 1;
    }

    // 2. Transform using EDR Engine
    ocr::representation::RepresentationConfig config;
    ocr::representation::DocumentRepresentationEngine engine(config);

    auto transformRes = engine.transform(rawJson);
    if (!transformRes) {
        std::cerr << "Error: EDR Transformation failed: " << transformRes.error().message << "\n";
        return 1;
    }

    const auto& doc = transformRes.value();
    nlohmann::json fullJson = ocr::representation::EdrJsonSerializer::toFullJson(doc);
    nlohmann::json legacyLlmJson = ocr::representation::EdrJsonSerializer::toLlmJson(doc, false);

    // 3. Build Loss-Aware LLM Context and Report
    ocr::context::LlmContextConfig ctxConfig;
    ctxConfig.tokenBudget = tokenBudget;
    ocr::context::LlmContextBuilder ctxBuilder(ctxConfig);

    auto ctxRes = ctxBuilder.build(fullJson, &rawJson);
    if (!ctxRes) {
        std::cerr << "Error: LLM Context Builder failed: " << ctxRes.error().message << "\n";
        return 1;
    }

    const auto& llmContext = ctxRes.value().context;
    const auto& llmReport = ctxRes.value().report;
    nlohmann::json contextJson = llmContext.toJson(false);
    nlohmann::json reportJson = llmReport.toJson();

    // Save outputs if requested
    if (!fullPath.empty()) {
        ocr::representation::EdrJsonSerializer::saveToFile(fullPath, fullJson);
    }
    if (!llmPath.empty()) {
        ocr::representation::EdrJsonSerializer::saveToFile(llmPath, legacyLlmJson);
    }
    if (!contextPath.empty()) {
        ocr::context::LlmContextSerializer::saveToFile(contextPath, contextJson);
    }
    if (!reportPath.empty()) {
        ocr::context::LlmContextSerializer::saveToFile(reportPath, reportJson);
    }

    if (!outDir.empty()) {
        std::filesystem::path docDir = ocr::context::ArtifactPaths::getDocumentDirectory(outDir, rawFilePath);
        std::error_code ec;
        std::filesystem::create_directories(docDir, ec);

        ocr::representation::EdrJsonSerializer::saveToFile(ocr::context::ArtifactPaths::rawOcrPath(docDir), rawJson);
        ocr::representation::EdrJsonSerializer::saveToFile(ocr::context::ArtifactPaths::edrFullPath(docDir), fullJson);
        ocr::context::LlmContextSerializer::saveToFile(ocr::context::ArtifactPaths::llmContextPath(docDir), contextJson);
        ocr::context::LlmContextSerializer::saveToFile(ocr::context::ArtifactPaths::llmContextReportPath(docDir), reportJson);
    }

    std::cout << "\n======================================================================\n"
              << "     OCR Document Representation & Loss-Aware Context Analysis       \n"
              << "======================================================================\n"
              << "Document ID          : " << doc.metadata.id << "\n"
              << "Document Type        : " << doc.documentType << "\n"
              << "Total Pages          : " << doc.metadata.pageCount << "\n\n"
              << "Payload & Token Overview:\n"
              << "  Raw OCR            : " << (llmReport.compression.rawBytes / 1024.0) << " KB (~" << llmReport.compression.estimatedRawTokens << " tokens)\n"
              << "  Full EDR           : " << (llmReport.compression.edrFullBytes / 1024.0) << " KB (~" << llmReport.compression.estimatedFullTokens << " tokens)\n"
              << "  Loss-Aware LLM Ctx : " << (llmReport.compression.llmContextBytes / 1024.0) << " KB (~" << llmReport.compression.estimatedLlmTokens << " tokens)\n"
              << "  Token Budget       : " << tokenBudget << " tokens\n"
              << "  Semantic Chunking  : " << (llmReport.isChunked ? ("ACTIVE (" + std::to_string(llmReport.chunkCount) + " chunks)") : "SINGLE_PAYLOAD") << "\n\n"
              << "Preservation & Quality Audit:\n"
              << "  Context Quality    : " << std::fixed << std::setprecision(1) << llmReport.contextQualityScore << " / 100.0\n"
              << "  Audit Status       : " << llmReport.status << "\n"
              << "  Page Coverage      : " << llmReport.pageCoverage.represented << "/" << llmReport.pageCoverage.detected << " (" << std::fixed << std::setprecision(1) << llmReport.pageCoverage.coveragePercentage << "%)\n"
              << "  Section Coverage   : " << llmReport.sectionCoverage.sectionsRepresented << "/" << llmReport.sectionCoverage.sectionsDetected << " (" << std::fixed << std::setprecision(1) << llmReport.sectionCoverage.coveragePercentage << "%)\n"
              << "  Table Coverage     : " << llmReport.tableCoverage.tablesRepresented << "/" << llmReport.tableCoverage.tablesDetected << " tables ("
                                         << llmReport.tableCoverage.rowsRepresented << "/" << llmReport.tableCoverage.rowsDetected << " rows, "
                                         << std::fixed << std::setprecision(1) << llmReport.tableCoverage.rowCoveragePercentage << "%)\n"
              << "  Meaningful Text    : " << std::fixed << std::setprecision(1) << llmReport.compression.meaningfulTextCoveragePercentage << "%\n"
              << "  Confidence Preserv : " << llmReport.confidencePreservation.sourceItemsWithPreservedEvidence << "/" << llmReport.confidencePreservation.sourceLowConfidenceItems << " ("
                                         << std::fixed << std::setprecision(1) << llmReport.confidencePreservation.preservationPercentage << "%, Annotations: "
                                         << llmReport.confidencePreservation.outputConfidenceAnnotations << ")\n"
              << "  Unaccounted Loss   : " << llmReport.lossAnalysis.unaccountedLossItems << " item(s)\n";

    if (!doc.identity.empty()) {
        std::cout << "\nExtracted Identity:\n";
        for (const auto& [k, v] : doc.identity) {
            std::cout << "  - " << std::left << std::setw(20) << k << ": " << v << "\n";
        }
    }

    if (!doc.institution.empty()) {
        std::cout << "\nExtracted Institution:\n";
        for (const auto& [k, v] : doc.institution) {
            std::cout << "  - " << std::left << std::setw(20) << k << ": " << v << "\n";
        }
    }

    std::cout << "======================================================================\n\n";

    return 0;
}
