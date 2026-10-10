#include <ocr/representation/DocumentRepresentationEngine.hpp>
#include <ocr/ErrorCode.hpp>

#include "BlockGrouper.hpp"
#include "ConfidenceAggregator.hpp"
#include "EdrJsonSerializer.hpp"
#include "LineGrouper.hpp"
#include "MultiPageContinuity.hpp"
#include "NoiseReducer.hpp"
#include "ReadingOrder.hpp"
#include "SectionDetector.hpp"
#include "SemanticCompressor.hpp"
#include "TableDetector.hpp"
#include "TableReconstructor.hpp"
#include "TextNormalizer.hpp"

#include <fstream>

namespace ocr::representation {

struct DocumentRepresentationEngine::Impl {
    RepresentationConfig config;

    explicit Impl(RepresentationConfig cfg) : config(std::move(cfg)) {}

    Result<SemanticDocument> transform(const OcrResult& ocrResult) const {
        SemanticDocument doc;
        doc.schemaVersion = "edr-0.2";
        doc.metadata = ocrResult.document;
        doc.engine = ocrResult.engine;
        doc.preflight = ocrResult.preflight;
        doc.documentClassification = ocrResult.documentClassification;
        doc.strategy = ocrResult.strategy;

        // Determine document type
        if (config.documentType != "auto" && !config.documentType.empty()) {
            doc.documentType = config.documentType;
        } else {
            // Heuristic detection: check for transcript keywords
            std::string fullText = TextNormalizer::toUpper(ocrResult.toPlainText());
            if (TextNormalizer::icontains(fullText, "TRANSCRIPT") ||
                TextNormalizer::icontains(fullText, "ACADEMIC RECORD") ||
                TextNormalizer::icontains(fullText, "COURSE") ||
                TextNormalizer::icontains(fullText, "CREDITS") ||
                TextNormalizer::icontains(fullText, "GPA") ||
                TextNormalizer::icontains(fullText, "DEGREE")) {
                doc.documentType = "transcript";
            } else {
                doc.documentType = "generic";
            }
        }

        std::vector<SemanticTable> pageTables;
        std::vector<RepresentationBlock> allBlocks;

        int tableCounter = 1;

        for (const auto& page : ocrResult.pages) {
            SemanticPage sPage;
            sPage.pageNumber = page.pageNumber;
            sPage.width = page.width;
            sPage.height = page.height;
            sPage.rotation = page.rotation;
            sPage.sourceType = page.sourceType;
            sPage.processingStrategy = page.processingStrategy;
            sPage.pageClassification = page.pageClassification;
            sPage.timing = page.timing;
            sPage.complexity = page.complexity;

            // 1. Collect all lines from blocks
            std::vector<OcrLine> pageLines;
            for (const auto& b : page.blocks) {
                for (const auto& l : b.lines) {
                    pageLines.push_back(l);
                }
            }

            // 2. Sort reading order
            pageLines = ReadingOrder::sortLines(pageLines);

            // 3. Filter noise
            pageLines = NoiseReducer::filterLines(pageLines, config.noiseReduction);

            // 4. Detect table regions
            auto tableCandidates = TableDetector::detectTableRegions(pageLines, config.tables);

            // 5. Reconstruct tables
            for (const auto& cand : tableCandidates) {
                SemanticTable tbl = TableReconstructor::reconstructTable(
                    pageLines, cand, page.pageNumber, tableCounter++, config.tables
                );
                if (!tbl.empty()) {
                    pageTables.push_back(std::move(tbl));
                }
            }

            // 6. Group lines into representation blocks
            auto blocks = BlockGrouper::groupLines(
                pageLines, page.pageNumber, page.width, page.height, config
            );

            for (auto& blk : blocks) {
                sPage.confidence.merge(blk.confidence);
                allBlocks.push_back(blk);
                sPage.blocks.push_back(std::move(blk));
            }

            doc.pages.push_back(std::move(sPage));
        }

        // 7. Multi-page continuity merge for tables
        doc.tables = MultiPageContinuity::mergeContinuousTables(std::move(pageTables), config.tables);

        // 8. Detect sections and extract structured fields
        doc.sections = SectionDetector::detectSections(allBlocks, doc.tables, config.sections);

        // 9. Extract high-level global entities (Identity, Institution, Academic)
        SectionDetector::extractGlobalEntities(doc.sections, doc.identity, doc.institution, doc.academic);

        // 10. Aggregate confidence
        doc.confidence = ConfidenceAggregator::aggregateDocumentConfidence(doc);

        // 11. Low-confidence warnings
        doc.warnings = ConfidenceAggregator::findLowConfidenceRegions(
            doc, static_cast<float>(config.compression.lowConfidenceThreshold)
        );

        // 12. Compute compression & token metrics
        nlohmann::json fullJson = EdrJsonSerializer::toFullJson(doc);
        nlohmann::json llmJson = EdrJsonSerializer::toLlmJson(doc, config.sourceReferences.llmEdr);

        doc.compression = SemanticCompressor::computeMetrics(ocrResult, fullJson, llmJson);

        return Result<SemanticDocument>::success(std::move(doc));
    }
};

DocumentRepresentationEngine::DocumentRepresentationEngine(RepresentationConfig config)
    : config_(std::move(config)),
      pimpl_(std::make_unique<Impl>(config_)) {}

DocumentRepresentationEngine::~DocumentRepresentationEngine() = default;
DocumentRepresentationEngine::DocumentRepresentationEngine(DocumentRepresentationEngine&&) noexcept = default;
DocumentRepresentationEngine& DocumentRepresentationEngine::operator=(DocumentRepresentationEngine&&) noexcept = default;

Result<SemanticDocument> DocumentRepresentationEngine::transform(const OcrResult& ocrResult) const {
    return pimpl_->transform(ocrResult);
}

Result<SemanticDocument> DocumentRepresentationEngine::transform(const nlohmann::json& rawOcrJson) const {
    try {
        OcrResult ocrResult = OcrResult::fromJson(rawOcrJson);
        return pimpl_->transform(ocrResult);
    } catch (const std::exception& e) {
        return Result<SemanticDocument>::failure(
            ErrorCode::InvalidJson,
            std::string("Failed to parse raw OCR JSON: ") + e.what()
        );
    }
}

Result<SemanticDocument> DocumentRepresentationEngine::transform(const std::filesystem::path& rawOcrJsonPath) const {
    std::error_code ec;
    if (!std::filesystem::exists(rawOcrJsonPath, ec)) {
        return Result<SemanticDocument>::failure(
            ErrorCode::FileNotFound,
            "Raw OCR JSON file not found: " + rawOcrJsonPath.string(),
            rawOcrJsonPath.string()
        );
    }

    std::ifstream file(rawOcrJsonPath);
    if (!file.is_open() || !file.good()) {
        return Result<SemanticDocument>::failure(
            ErrorCode::FileReadError,
            "Failed to open raw OCR JSON file: " + rawOcrJsonPath.string(),
            rawOcrJsonPath.string()
        );
    }

    try {
        nlohmann::json j;
        file >> j;
        return transform(j);
    } catch (const std::exception& e) {
        return Result<SemanticDocument>::failure(
            ErrorCode::InvalidJson,
            std::string("JSON parse exception: ") + e.what(),
            rawOcrJsonPath.string()
        );
    }
}

Result<DocumentRepresentationEngine::StoredRepresentationPaths> DocumentRepresentationEngine::storeRawAndCreateEdr(
    const OcrResult& ocrResult,
    const std::filesystem::path& outputDir,
    const std::string& fileStem,
    bool includeSourceInLlm
) const {
    (void)fileStem;
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);

    // 1. Transform directly from in-memory OCR Result (Linear pipeline)
    auto reprRes = transform(ocrResult);
    if (!reprRes) {
        return Result<StoredRepresentationPaths>::failure(reprRes.error());
    }

    const auto& semDoc = reprRes.value();

    // 2. Store Raw OCR JSON to disk
    std::filesystem::path rawPath = outputDir / "raw_ocr.json";
    nlohmann::json rawJson = ocrResult.toJson(OutputFormat::FullJson);
    if (!EdrJsonSerializer::saveToFile(rawPath, rawJson)) {
        return Result<StoredRepresentationPaths>::failure(
            ErrorCode::SerializationFailed,
            "Failed to store raw OCR JSON to: " + rawPath.string(),
            rawPath.string()
        );
    }

    // 3. Serialize and save Full EDR JSON
    std::filesystem::path fullPath = outputDir / "edr_full.json";
    nlohmann::json fullJson = toFullJson(semDoc);
    if (!EdrJsonSerializer::saveToFile(fullPath, fullJson)) {
        return Result<StoredRepresentationPaths>::failure(
            ErrorCode::SerializationFailed,
            "Failed to store Full EDR JSON to: " + fullPath.string(),
            fullPath.string()
        );
    }

    // 4. Serialize and save legacy LLM EDR JSON
    std::filesystem::path llmPath = outputDir / "edr_llm.json";
    nlohmann::json llmJson = toLlmJson(semDoc, includeSourceInLlm);
    if (!EdrJsonSerializer::saveToFile(llmPath, llmJson)) {
        return Result<StoredRepresentationPaths>::failure(
            ErrorCode::SerializationFailed,
            "Failed to store LLM EDR JSON to: " + llmPath.string(),
            llmPath.string()
        );
    }

    return Result<StoredRepresentationPaths>::success(
        StoredRepresentationPaths{
            std::move(rawPath),
            std::move(fullPath),
            std::move(llmPath),
            semDoc
        }
    );
}

nlohmann::json DocumentRepresentationEngine::toFullJson(const SemanticDocument& doc) {
    return EdrJsonSerializer::toFullJson(doc);
}

nlohmann::json DocumentRepresentationEngine::toLlmJson(const SemanticDocument& doc, bool includeSource) {
    return EdrJsonSerializer::toLlmJson(doc, includeSource);
}

} // namespace ocr::representation

