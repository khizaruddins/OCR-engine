#include <ocr/OcrEngine.hpp>
#include <ocr/ErrorCode.hpp>
#include <ocr/context/ArtifactManager.hpp>
#include <ocr/context/ArtifactPaths.hpp>
#include <ocr/context/LlmContextBuilder.hpp>
#include <ocr/ingestion/PreflightReport.hpp>
#include <ocr/representation/DocumentRepresentationEngine.hpp>
#include <ocr/representation/EdrJsonSerializer.hpp>
#include "../../src/context/LlmContextSerializer.hpp"
#include "../../src/output/JsonSerializer.hpp"
#include "../../src/utils/FileUtils.hpp"
#include "../../src/logging/Logger.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

void printVersion() {
    std::cout << "EDMO Document Processing & OCR Engine v0.2.0\n"
              << "Copyright (C) 2026 EDMO Team\n"
              << "Core Architecture: Document Preflight | Page Classification | Native Text & Tesseract OCR | EDR v0.2\n";
}

void printHelp(const char* progName) {
    std::cout << "EDMO Document Processing & OCR Engine v0.2.0\n\n"
              << "Usage:\n"
              << "  " << progName << " <input-file> [options]\n"
              << "  " << progName << " <input-dir> --batch [options]\n\n"
              << "Options:\n"
              << "  -i, --input <path>               Input image/PDF file or directory\n"
              << "  -o, --output <path>              Output file destination or directory (for batch)\n"
              << "      --output-dir <path>          Output directory for generated artifacts (default: output/documents)\n"
              << "  -f, --format <format>            Output format: full-json, compact-json, text (default: full-json)\n"
              << "  -r, --representation <mode>      Representation mode: none, edr, llm, llm-context, llm-compact, llm-traceable, both, all (default: all)\n"
              << "      --preflight-only             Run document inspection and page classification only\n"
              << "      --include-preflight          Include preflight inspection report in output\n"
              << "      --preflight-output <path>    Write preflight JSON report to specified destination\n"
              << "      --force-ocr                  Force OCR path for all pages (bypasses native text)\n"
              << "      --force-native-text          Force native text extraction for all available pages\n"
              << "      --parallel-pages             Enable page-level parallelism where available\n"
              << "      --benchmark                  Print stage-by-stage latency and performance metrics\n"
              << "      --llm-token-budget <int>     Token budget for LLM Context Builder (default: 12000)\n"
              << "      --document-type <type>       Document classification: auto, transcript, generic, resume (default: auto)\n"
              << "      --include-source             Include bounding box source references in LLM EDR output\n"
              << "  -d, --dpi <int>                  Rendering DPI for PDF processing (default: 300)\n"
              << "  -l, --lang <lang>                OCR language(s) (default: eng)\n"
              << "      --psm <int>                  Page segmentation mode (0-13, default: 3)\n"
              << "      --oem <int>                  OCR engine mode (0-3, default: 3)\n"
              << "      --no-preprocessing           Disable image preprocessing pipeline\n"
              << "      --no-deskew                  Disable automatic deskewing\n"
              << "      --no-denoise                 Disable automatic denoising\n"
              << "      --no-threshold               Disable adaptive binarization\n"
              << "      --no-orientation             Disable automatic orientation detection\n"
              << "      --contrast <float>           Contrast adjustment factor (default: 1.0)\n"
              << "      --brightness <int>           Brightness adjustment offset (default: 0)\n"
              << "  -c, --config <path>              Load configuration from JSON file\n"
              << "  -b, --batch                      Batch process all supported files in directory\n"
              << "      --max-pages <int>            Maximum pages to process in PDF (default: 0 = all)\n"
              << "  -v, --verbose                    Enable verbose logging output\n"
              << "  -q, --quiet                      Suppress log output, print results only\n"
              << "      --version                    Display version information\n"
              << "  -h, --help                       Display this help message\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp(argv[0]);
        return 1;
    }

    std::string inputPath;
    std::string outputPath;
    std::string outputDir;
    std::string configPath;
    std::string preflightOutputPath;
    std::string customDocumentId;
    std::string reprMode = "all";
    std::string docType = "auto";
    bool includeSource = false;
    size_t llmTokenBudget = 12000;
    ocr::OutputFormat format = ocr::OutputFormat::FullJson;
    bool isBatch = false;
    bool verbose = false;
    bool quiet = false;
    bool preflightOnly = false;
    bool includePreflight = false;
    bool forceOcr = false;
    bool forceNativeText = false;
    bool parallelPages = false;
    bool benchmark = false;

    ocr::OcrConfig config;
    bool customDpi = false;
    int dpiValue = 300;
    bool customLang = false;
    std::string langValue;
    bool customPsm = false;
    int psmValue = 3;
    bool customOem = false;
    int oemValue = 3;
    bool noPreprocess = false;
    bool noDeskew = false;
    bool noDenoise = false;
    bool noThreshold = false;
    bool noOrientation = false;
    bool customMaxPages = false;
    int maxPagesValue = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--version") {
            printVersion();
            return 0;
        } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "--document-id" && i + 1 < argc) {
            customDocumentId = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            outputDir = argv[++i];
        } else if ((arg == "-f" || arg == "--format") && i + 1 < argc) {
            std::string fmtStr = argv[++i];
            if (fmtStr == "full-json" || fmtStr == "json") {
                format = ocr::OutputFormat::FullJson;
            } else if (fmtStr == "compact-json" || fmtStr == "compact") {
                format = ocr::OutputFormat::CompactJson;
            } else if (fmtStr == "text" || fmtStr == "txt") {
                format = ocr::OutputFormat::PlainText;
            }
        } else if ((arg == "-r" || arg == "--representation") && i + 1 < argc) {
            reprMode = argv[++i];
        } else if (arg == "--preflight-only") {
            preflightOnly = true;
        } else if (arg == "--include-preflight") {
            includePreflight = true;
        } else if (arg == "--preflight-output" && i + 1 < argc) {
            preflightOutputPath = argv[++i];
        } else if (arg == "--force-ocr") {
            forceOcr = true;
        } else if (arg == "--force-native-text") {
            forceNativeText = true;
        } else if (arg == "--parallel-pages") {
            parallelPages = true;
        } else if (arg == "--benchmark") {
            benchmark = true;
        } else if (arg == "--llm-token-budget" && i + 1 < argc) {
            llmTokenBudget = static_cast<size_t>(std::stoul(argv[++i]));
        } else if (arg == "--document-type" && i + 1 < argc) {
            docType = argv[++i];
        } else if (arg == "--include-source") {
            includeSource = true;
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg == "-b" || arg == "--batch") {
            isBatch = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if ((arg == "-d" || arg == "--dpi") && i + 1 < argc) {
            customDpi = true;
            dpiValue = std::atoi(argv[++i]);
        } else if ((arg == "-l" || arg == "--lang") && i + 1 < argc) {
            customLang = true;
            langValue = argv[++i];
        } else if (arg == "--psm" && i + 1 < argc) {
            customPsm = true;
            psmValue = std::atoi(argv[++i]);
        } else if (arg == "--oem" && i + 1 < argc) {
            customOem = true;
            oemValue = std::atoi(argv[++i]);
        } else if (arg == "--no-preprocessing") {
            noPreprocess = true;
        } else if (arg == "--no-deskew") {
            noDeskew = true;
        } else if (arg == "--no-denoise") {
            noDenoise = true;
        } else if (arg == "--no-threshold") {
            noThreshold = true;
        } else if (arg == "--no-orientation") {
            noOrientation = true;
        } else if (arg == "--max-pages" && i + 1 < argc) {
            customMaxPages = true;
            maxPagesValue = std::atoi(argv[++i]);
        } else if (arg[0] != '-' && inputPath.empty()) {
            inputPath = arg;
        }
    }

    if (inputPath.empty()) {
        std::cerr << "Error: No input file or directory specified.\n";
        return 1;
    }

    if (quiet) {
        ocr::Logger::setLevel(ocr::LogLevel::Off);
    } else if (verbose) {
        ocr::Logger::setLevel(ocr::LogLevel::Debug);
    } else {
        ocr::Logger::setLevel(ocr::LogLevel::Info);
    }

    if (!configPath.empty()) {
        auto cfgRes = ocr::OcrConfig::fromFile(configPath);
        if (cfgRes) config = cfgRes.value();
    }

    if (customDpi) config.pdf.dpi = dpiValue;
    if (customLang) config.ocr.language = langValue;
    if (customPsm) config.ocr.psm = psmValue;
    if (customOem) config.ocr.oem = oemValue;
    if (noPreprocess) config.preprocessing.enabled = false;
    if (noDeskew) config.preprocessing.deskew = false;
    if (noDenoise) config.preprocessing.denoise = false;
    if (noThreshold) config.preprocessing.binarization = false;
    if (noOrientation) config.preprocessing.orientation = false;
    if (customMaxPages) config.pdf.maxPages = maxPagesValue;

    if (forceOcr) config.ingestion.forceOcr = true;
    if (forceNativeText) config.ingestion.forceNativeText = true;
    if (parallelPages) config.ingestion.enablePageParallelism = true;

    ocr::OcrEngine engine(config);

    // Preflight Only Mode
    if (preflightOnly) {
        auto preflightRes = engine.preflight(inputPath);
        if (!preflightRes) {
            std::cerr << "Preflight error: " << preflightRes.error().message << "\n";
            return 1;
        }

        std::string reportJson = ocr::ingestion::PreflightReport::serialize(preflightRes.value(), true);
        if (!preflightOutputPath.empty()) {
            if (ocr::ingestion::PreflightReport::saveToFile(preflightOutputPath, preflightRes.value(), true)) {
                std::cout << "Preflight report written to: " << preflightOutputPath << "\n";
            }
        } else if (!outputPath.empty()) {
            if (ocr::ingestion::PreflightReport::saveToFile(outputPath, preflightRes.value(), true)) {
                std::cout << "Preflight report written to: " << outputPath << "\n";
            }
        } else {
            std::cout << reportJson << "\n";
        }
        return 0;
    }

    ocr::representation::RepresentationConfig reprConfig;
    reprConfig.documentType = docType;
    reprConfig.sourceReferences.llmEdr = includeSource;
    ocr::representation::DocumentRepresentationEngine reprEngine(reprConfig);

    std::filesystem::path baseOutputDir = !outputDir.empty() ? outputDir : "output/documents";

    if (isBatch || std::filesystem::is_directory(inputPath)) {
        auto files = ocr::FileUtils::listSupportedFiles(inputPath, true);
        if (files.empty()) {
            std::cout << "No supported documents found in: " << inputPath << "\n";
            return 0;
        }

        std::error_code ec;
        std::filesystem::create_directories(baseOutputDir, ec);

        std::cout << "========================================\n"
                  << "EDMO Document Processing & OCR Batch Execution\n"
                  << "Total Files: " << files.size() << "\n"
                  << "========================================\n";

        for (const auto& file : files) {
            auto result = engine.process(file);
            if (result) {
                if (reprMode == "none") {
                    std::string stem = file.stem().string();
                    std::string ext = (format == ocr::OutputFormat::PlainText) ? ".txt" : ".json";
                    ocr::JsonSerializer::saveToFile(baseOutputDir / (stem + ext), result.value(), format, true);
                } else {
                    auto reprRes = reprEngine.transform(result.value());
                    if (reprRes) {
                        ocr::context::LlmContextConfig ctxCfg;
                        ctxCfg.tokenBudget = llmTokenBudget;
                        if (reprMode == "llm-traceable") ctxCfg.mode = ocr::context::ContextMode::Traceable;
                        ocr::context::LlmContextBuilder ctxBuilder(ctxCfg);
                        auto ctxRes = ctxBuilder.build(reprRes.value(), &result.value());
                        if (ctxRes) {
                            ocr::context::ArtifactManager::writeAll(baseOutputDir, file.filename().string(), &result.value(), reprRes.value(), ctxRes.value().context, ctxRes.value().report, true);
                        }
                    }
                }
                std::cout << "Processed: " << file.filename().string() << "\n";
            }
        }
    } else {
        auto result = engine.process(inputPath);
        if (!result) {
            std::cerr << "Processing failed: " << result.error().message << "\n";
            return 1;
        }

        std::filesystem::path inPath(inputPath);
        if (!customDocumentId.empty()) {
            result.value().document.id = customDocumentId;
        }

        if (reprMode == "none") {
            if (!outputPath.empty()) ocr::JsonSerializer::saveToFile(outputPath, result.value(), format, true);
            else std::cout << ocr::JsonSerializer::serialize(result.value(), format, true) << "\n";
            return 0;
        }

        auto reprRes = reprEngine.transform(result.value());
        if (!customDocumentId.empty() && reprRes) {
            reprRes.value().metadata.id = customDocumentId;
        }

        ocr::context::LlmContextConfig ctxCfg;
        ctxCfg.tokenBudget = llmTokenBudget;
        if (reprMode == "llm-traceable") ctxCfg.mode = ocr::context::ContextMode::Traceable;
        ocr::context::LlmContextBuilder ctxBuilder(ctxCfg);
        auto ctxRes = ctxBuilder.build(reprRes.value(), &result.value());
        if (!customDocumentId.empty() && ctxRes) {
            ctxRes.value().report.documentId = customDocumentId;
        }

        auto written = ocr::context::ArtifactManager::writeAll(baseOutputDir, inPath.filename().string(), &result.value(), reprRes.value(), ctxRes.value().context, ctxRes.value().report, true);

        if (!preflightOutputPath.empty()) {
            (void)ocr::ingestion::PreflightReport::saveToFile(preflightOutputPath, result.value().preflight, true);
        } else if (includePreflight) {
            (void)ocr::ingestion::PreflightReport::saveToFile(written.documentDirectory / "preflight.json", result.value().preflight, true);
        }

        if (!outputPath.empty()) {
            if (reprMode == "edr") ocr::representation::EdrJsonSerializer::saveToFile(outputPath, ocr::representation::EdrJsonSerializer::toFullJson(reprRes.value()));
            else ocr::context::LlmContextSerializer::saveToFile(outputPath, ctxRes.value().context.toJson(reprMode == "llm-traceable"));
        }

        const auto& rep = ctxRes.value().report;
        const auto& res = result.value();

        int nativePages = 0;
        int ocrPages = 0;
        for (const auto& p : res.pages) {
            if (p.sourceType == "NATIVE_TEXT") nativePages++;
            else ocrPages++;
        }

        std::cout << "======================================================================\n"
                  << "EDMO Document Processing & OCR Engine v0.2.0 Summary\n"
                  << "======================================================================\n"
                  << "Document Directory    : " << written.documentDirectory.string() << "\n"
                  << "Document Classification: " << res.documentClassification << " (Strategy: " << res.strategy << ")\n"
                  << "Page Processing Split : Native Text: " << nativePages << " | OCR: " << ocrPages << "\n"
                  << "Artifacts Generated   :\n"
                  << "  • Raw OCR JSON      : " << ocr::context::ArtifactPaths::kRawOcrFilename << "\n"
                  << "  • Full EDR JSON     : " << ocr::context::ArtifactPaths::kEdrFullFilename << "\n"
                  << "  • LLM Context JSON  : " << ocr::context::ArtifactPaths::kLlmContextFilename << "\n"
                  << "  • LLM Context Report: " << ocr::context::ArtifactPaths::kLlmContextReportFilename << "\n";

        if (benchmark) {
            std::cout << "----------------------------------------------------------------------\n"
                      << "Performance & Latency Breakdown:\n"
                      << "  • Preflight / Ingestion : " << std::fixed << std::setprecision(2) << res.timing.preflightTimeMs << " ms\n"
                      << "  • Native Text Extraction: " << std::fixed << std::setprecision(2) << res.timing.nativeExtractionTimeMs << " ms\n"
                      << "  • PDF Rendering (for OCR):" << std::fixed << std::setprecision(2) << res.timing.pdfRenderTimeMs << " ms\n"
                      << "  • OCR Preprocessing     : " << std::fixed << std::setprecision(2) << res.timing.preprocessTimeMs << " ms\n"
                      << "  • OCR Recognition       : " << std::fixed << std::setprecision(2) << res.timing.ocrTimeMs << " ms\n"
                      << "  • Total Execution Time  : " << std::fixed << std::setprecision(2) << res.timing.totalTimeMs << " ms ("
                      << (res.pages.empty() ? 0.0 : (res.timing.totalTimeMs / res.pages.size())) << " ms/page)\n";
        }

        std::cout << "----------------------------------------------------------------------\n"
                  << "Preservation & Coverage Audit:\n"
                  << "  • Context Quality Score : " << std::fixed << std::setprecision(1) << rep.contextQualityScore << "/100.0\n"
                  << "  • Status                : " << rep.status << "\n"
                  << "  • Page Coverage         : " << rep.pageCoverage.represented << "/" << rep.pageCoverage.detected << " (" << std::fixed << std::setprecision(1) << rep.pageCoverage.coveragePercentage << "%)\n"
                  << "  • Section Coverage      : " << rep.sectionCoverage.sectionsRepresented << "/" << rep.sectionCoverage.sectionsDetected << " (" << std::fixed << std::setprecision(1) << rep.sectionCoverage.coveragePercentage << "%)\n"
                  << "  • Table Row Coverage    : " << rep.tableCoverage.rowsRepresented << "/" << rep.tableCoverage.rowsDetected << " (" << std::fixed << std::setprecision(1) << rep.tableCoverage.rowCoveragePercentage << "%)\n"
                  << "  • Confidence Preserv.   : " << rep.confidencePreservation.sourceItemsWithPreservedEvidence << "/" << rep.confidencePreservation.sourceLowConfidenceItems << " (" << std::fixed << std::setprecision(1) << rep.confidencePreservation.preservationPercentage << "%, Annotations: " << rep.confidencePreservation.outputConfidenceAnnotations << ")\n"
                  << "  • Meaningful Text Cov.  : " << std::fixed << std::setprecision(1) << rep.compression.meaningfulTextCoveragePercentage << "%\n"
                  << "  • Unaccounted Loss      : " << rep.lossAnalysis.unaccountedLossItems << " item(s)\n"
                  << "  • Est. Context Tokens   : " << rep.compression.estimatedLlmTokens << " tokens (Budget: " << llmTokenBudget << ")\n"
                  << "  • Semantic Chunking     : " << (rep.isChunked ? ("ACTIVE (" + std::to_string(rep.chunkCount) + " chunks)") : "SINGLE_PAYLOAD") << "\n"
                  << "======================================================================\n";
    }
    return 0;
}
