#pragma once

#include <edmo/ocr/OcrBlock.hpp>
#include <edmo/ocr/Types.hpp>
#include <edmo/ocr/ingestion/IngestionTypes.hpp>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::ocr {

/**
 * @brief Timing performance breakdown for processing stages.
 */
struct TimingBreakdown {
    double pdfRenderTimeMs = 0.0;
    double preprocessTimeMs = 0.0;
    double ocrTimeMs = 0.0;
    double serializationTimeMs = 0.0;
    double totalTimeMs = 0.0;
    double preflightTimeMs = 0.0;
    double nativeExtractionTimeMs = 0.0;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"pdf_render_time_ms", pdfRenderTimeMs},
            {"preflight_time_ms", preflightTimeMs},
            {"native_extraction_time_ms", nativeExtractionTimeMs},
            {"preprocess_time_ms", preprocessTimeMs},
            {"ocr_time_ms", ocrTimeMs},
            {"serialization_time_ms", serializationTimeMs},
            {"total_time_ms", totalTimeMs}
        };
    }
    static TimingBreakdown fromJson(const nlohmann::json& j) {
        TimingBreakdown tb;
        tb.pdfRenderTimeMs = j.value("pdf_render_time_ms", 0.0);
        tb.preflightTimeMs = j.value("preflight_time_ms", 0.0);
        tb.nativeExtractionTimeMs = j.value("native_extraction_time_ms", 0.0);
        tb.preprocessTimeMs = j.value("preprocess_time_ms", 0.0);
        tb.ocrTimeMs = j.value("ocr_time_ms", 0.0);
        tb.serializationTimeMs = j.value("serialization_time_ms", 0.0);
        tb.totalTimeMs = j.value("total_time_ms", 0.0);
        return tb;
    }
};

/**
 * @brief OCR Engine metadata.
 */
struct EngineInfo {
    std::string name = "edmo-ocr";
    std::string version = "0.2.0";
    std::string provider = "tesseract";

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"name", name},
            {"version", version},
            {"provider", provider}
        };
    }

    static EngineInfo fromJson(const nlohmann::json& j) {
        EngineInfo info;
        info.name = j.value("name", "edmo-ocr");
        info.version = j.value("version", "0.2.0");
        info.provider = j.value("provider", "tesseract");
        return info;
    }
};

/**
 * @brief Document identification and source metadata.
 */
struct DocumentMetadata {
    std::string id;
    std::string source;
    std::string type; // "pdf" or "image"
    int pageCount = 1;
    size_t fileSizeBytes = 0;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"id", id},
            {"source", source},
            {"type", type},
            {"page_count", pageCount},
            {"file_size_bytes", fileSizeBytes}
        };
    }

    static DocumentMetadata fromJson(const nlohmann::json& j) {
        DocumentMetadata meta;
        meta.id = j.value("id", "");
        meta.source = j.value("source", "");
        meta.type = j.value("type", "pdf");
        meta.pageCount = j.value("page_count", 1);
        meta.fileSizeBytes = j.value("file_size_bytes", size_t(0));
        return meta;
    }
};

/**
 * @brief Page-level OCR result container.
 */
struct OcrPageResult {
    int pageNumber = 1;
    int width = 0;
    int height = 0;
    int rotation = 0; // 0, 90, 180, 270
    double skewAngle = 0.0;
    float confidence = 0.0f; // Normalized 0.0 to 1.0

    std::string rawText;
    std::string normalizedText;

    std::vector<OcrBlock> blocks;
    std::vector<OcrBlock> nativeBlocks;
    bool hasConflict = false;
    std::string conflictReason;

    double renderTimeMs = 0.0;
    double preprocessTimeMs = 0.0;
    double ocrTimeMs = 0.0;

    // Processing & ingestion metadata
    std::string sourceType = "OCR"; // "OCR" or "NATIVE_TEXT"
    std::string processingStrategy = "OCR"; // "NATIVE_TEXT", "OCR", "OCR_ENHANCED", etc.
    std::string pageClassification = "SCANNED_IMAGE"; // "DIGITAL_TEXT", "SCANNED_IMAGE", etc.
    ingestion::PageTiming timing;
    ingestion::PageComplexity complexity;

    [[nodiscard]] nlohmann::json toJson(OutputFormat format = OutputFormat::FullJson) const {
        nlohmann::json j{
            {"page_number", pageNumber},
            {"width", width},
            {"height", height},
            {"rotation", rotation},
            {"skew_angle", skewAngle},
            {"confidence", confidence},
            {"text", normalizedText.empty() ? rawText : normalizedText},
            {"processing", {
                {"source_type", sourceType},
                {"processing_strategy", processingStrategy},
                {"page_classification", pageClassification},
                {"timing", timing.toJson()},
                {"complexity", complexity.toJson()}
            }}
        };

        if (hasConflict) {
            j["has_conflict"] = true;
            j["conflict_reason"] = conflictReason;
        }

        if (format == OutputFormat::FullJson) {
            j["raw_text"] = rawText;
            nlohmann::json blocksJson = nlohmann::json::array();
            for (const auto& block : blocks) {
                blocksJson.push_back(block.toJson(true, true));
            }
            j["blocks"] = std::move(blocksJson);

            if (!nativeBlocks.empty()) {
                nlohmann::json nbJson = nlohmann::json::array();
                for (const auto& nb : nativeBlocks) {
                    nbJson.push_back(nb.toJson(true, true));
                }
                j["native_blocks"] = std::move(nbJson);
            }

            j["timing"] = {
                {"render_time_ms", renderTimeMs},
                {"preprocess_time_ms", preprocessTimeMs},
                {"ocr_time_ms", ocrTimeMs},
                {"preflight_ms", timing.preflightMs},
                {"native_extraction_ms", timing.nativeExtractionMs},
                {"total_ms", timing.totalMs}
            };
        }

        return j;
    }

    static OcrPageResult fromJson(const nlohmann::json& j) {
        OcrPageResult pr;
        pr.pageNumber = j.value("page_number", 1);
        pr.width = j.value("width", 0);
        pr.height = j.value("height", 0);
        pr.rotation = j.value("rotation", 0);
        pr.skewAngle = j.value("skew_angle", 0.0);
        pr.confidence = j.value("confidence", 0.0f);
        pr.rawText = j.value("raw_text", j.value("text", ""));
        pr.normalizedText = j.value("text", "");

        if (j.contains("processing") && j["processing"].is_object()) {
            const auto& proc = j["processing"];
            pr.sourceType = proc.value("source_type", "OCR");
            pr.processingStrategy = proc.value("processing_strategy", "OCR");
            pr.pageClassification = proc.value("page_classification", "SCANNED_IMAGE");
        }

        if (j.contains("blocks") && j["blocks"].is_array()) {
            for (const auto& b : j["blocks"]) {
                pr.blocks.push_back(OcrBlock::fromJson(b));
            }
        }
        if (j.contains("timing") && j["timing"].is_object()) {
            pr.renderTimeMs = j["timing"].value("render_time_ms", 0.0);
            pr.preprocessTimeMs = j["timing"].value("preprocess_time_ms", 0.0);
            pr.ocrTimeMs = j["timing"].value("ocr_time_ms", 0.0);
            pr.timing.preflightMs = j["timing"].value("preflight_ms", 0.0);
            pr.timing.nativeExtractionMs = j["timing"].value("native_extraction_ms", 0.0);
            pr.timing.ocrMs = pr.ocrTimeMs;
            pr.timing.totalMs = j["timing"].value("total_ms", 0.0);
        }
        return pr;
    }
};

/**
 * @brief Complete hierarchical OCR result structure adhering to EDMO schema v0.1.
 */
struct OcrResult {
    std::string id;
    std::string schemaVersion = "0.1";
    EngineInfo engine;
    DocumentMetadata document;
    std::vector<OcrPageResult> pages;
    float confidence = 0.0f; // Average page-level confidence
    TimingBreakdown timing;

    ingestion::PreflightSummary preflight;
    std::string documentClassification = "UNKNOWN";
    std::string strategy = "ADAPTIVE";

    [[nodiscard]] std::string toPlainText() const {
        std::string out;
        for (const auto& page : pages) {
            if (pages.size() > 1) {
                out += "--- Page " + std::to_string(page.pageNumber) + " ---\n";
            }
            out += page.normalizedText.empty() ? page.rawText : page.normalizedText;
            if (!out.empty() && out.back() != '\n') {
                out += '\n';
            }
        }
        return out;
    }

    [[nodiscard]] nlohmann::json toJson(OutputFormat format = OutputFormat::FullJson) const {
        nlohmann::json j{
            {"schema_version", schemaVersion},
            {"id", id},
            {"engine", engine.toJson()},
            {"document", document.toJson()},
            {"confidence", confidence}
        };

        nlohmann::json pagesJson = nlohmann::json::array();
        for (const auto& p : pages) {
            pagesJson.push_back(p.toJson(format));
        }
        j["pages"] = std::move(pagesJson);
        j["processing"] = timing.toJson();

        return j;
    }

    static OcrResult fromJson(const nlohmann::json& j) {
        OcrResult res;
        res.id = j.value("id", "");
        res.schemaVersion = j.value("schema_version", "0.1");
        if (j.contains("engine") && j["engine"].is_object()) {
            res.engine = EngineInfo::fromJson(j["engine"]);
        }
        if (j.contains("document") && j["document"].is_object()) {
            res.document = DocumentMetadata::fromJson(j["document"]);
        }
        res.confidence = j.value("confidence", 0.0f);

        if (j.contains("pages") && j["pages"].is_array()) {
            for (const auto& p : j["pages"]) {
                res.pages.push_back(OcrPageResult::fromJson(p));
            }
        }
        if (j.contains("processing") && j["processing"].is_object()) {
            res.timing = TimingBreakdown::fromJson(j["processing"]);
        }
        return res;
    }
};

} // namespace edmo::ocr
