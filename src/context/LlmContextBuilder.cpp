#include <edmo/ocr/context/LlmContextBuilder.hpp>
#include "../utils/Timer.hpp"

#include "LossAnalyzer.hpp"
#include "SafeDeduplicator.hpp"
#include "SemanticChunker.hpp"
#include "SemanticInventoryBuilder.hpp"
#include "TableCompactor.hpp"
#include "TextCompactor.hpp"
#include "LlmContextSerializer.hpp"

#include <fstream>

namespace edmo::ocr::context {

struct LlmContextBuilder::Impl {
    LlmContextConfig config;

    explicit Impl(LlmContextConfig cfg) : config(std::move(cfg)) {}

    Result<LlmContextBuildResult> build(
        const representation::SemanticDocument& doc,
        const nlohmann::json* optionalRawOcrJson
    ) {
        Timer timer;

        // 1. Build Comprehensive Inventory
        auto inventory = SemanticInventoryBuilder::build(doc);

        // 2. Safe Deduplication of repeated headers/footers
        auto dedupRes = SafeDeduplicator::deduplicate(doc, config.deduplication, inventory);

        // 3. Compact Tables
        auto contextTables = TableCompactor::compactTables(doc.tables, config, inventory);

        // 4. Compact Sections and Unclassified Content
        auto textRes = TextCompactor::compact(doc, dedupRes, config, inventory);

        // 5. Assemble LlmContext
        LlmContext context;
        context.schemaVersion = "llm-context-0.1";
        context.documentType = doc.documentType.empty() ? "transcript" : doc.documentType;
        context.pageCount = doc.metadata.pageCount > 0 ? doc.metadata.pageCount : 1;
        for (int p = 1; p <= context.pageCount; ++p) {
            context.pagesPresent.push_back(p);
        }
        context.language = "eng";

        context.documentHeader = std::move(dedupRes.documentHeader);
        context.documentFooter = std::move(dedupRes.documentFooter);

        for (const auto& [k, v] : doc.identity) {
            context.student[k] = v;
        }
        for (const auto& [k, v] : doc.institution) {
            context.institution[k] = v;
        }
        for (const auto& [k, v] : doc.academic) {
            context.academic[k] = v;
        }

        context.sections = std::move(textRes.sections);
        context.tables = std::move(contextTables);
        context.unclassifiedContent = std::move(textRes.unclassifiedContent);

        // 6. Apply Token Budgeting and Semantic Chunking
        SemanticChunker::applyBudgetAndChunking(context, config, inventory);

        // 7. Loss Analysis and Audit Reporting
        auto report = LossAnalyzer::analyze(doc, context, inventory, optionalRawOcrJson);
        report.totalExecutionTimeMs = timer.elapsedMs();

        LlmContextBuildResult buildRes;
        buildRes.context = std::move(context);
        buildRes.report = std::move(report);

        return Result<LlmContextBuildResult>::success(std::move(buildRes));
    }
};

LlmContextBuilder::LlmContextBuilder(LlmContextConfig config)
    : config_(std::move(config)),
      pimpl_(std::make_unique<Impl>(config_)) {}

LlmContextBuilder::~LlmContextBuilder() = default;
LlmContextBuilder::LlmContextBuilder(LlmContextBuilder&&) noexcept = default;
LlmContextBuilder& LlmContextBuilder::operator=(LlmContextBuilder&&) noexcept = default;

Result<LlmContextBuildResult> LlmContextBuilder::build(
    const representation::SemanticDocument& fullEdrDoc,
    const OcrResult* optionalRawOcr
) const {
    nlohmann::json rawJson;
    if (optionalRawOcr != nullptr) {
        rawJson = optionalRawOcr->toJson(OutputFormat::FullJson);
        return pimpl_->build(fullEdrDoc, &rawJson);
    }
    return pimpl_->build(fullEdrDoc, nullptr);
}

Result<LlmContextBuildResult> LlmContextBuilder::build(
    const nlohmann::json& fullEdrJson,
    const nlohmann::json* optionalRawOcrJson
) const {
    try {
        // Construct SemanticDocument from Full EDR JSON
        representation::SemanticDocument doc;
        doc.schemaVersion = fullEdrJson.value("schema_version", "edr-0.1");

        if (fullEdrJson.contains("document")) {
            const auto& d = fullEdrJson["document"];
            doc.metadata.id = d.value("id", "");
            doc.documentType = d.value("type", "transcript");
            doc.metadata.pageCount = d.value("page_count", 1);
            doc.metadata.source = d.value("source", "");
        }

        if (fullEdrJson.contains("identity")) {
            for (auto it = fullEdrJson["identity"].begin(); it != fullEdrJson["identity"].end(); ++it) {
                if (it.value().is_string()) {
                    doc.identity[it.key()] = it.value().get<std::string>();
                }
            }
        }

        if (fullEdrJson.contains("institution")) {
            for (auto it = fullEdrJson["institution"].begin(); it != fullEdrJson["institution"].end(); ++it) {
                if (it.value().is_string()) {
                    doc.institution[it.key()] = it.value().get<std::string>();
                }
            }
        }

        if (fullEdrJson.contains("academic")) {
            for (auto it = fullEdrJson["academic"].begin(); it != fullEdrJson["academic"].end(); ++it) {
                if (it.value().is_string()) {
                    doc.academic[it.key()] = it.value().get<std::string>();
                }
            }
        }

        if (fullEdrJson.contains("pages") && fullEdrJson["pages"].is_array()) {
            for (const auto& pj : fullEdrJson["pages"]) {
                representation::SemanticPage page;
                page.pageNumber = pj.value("page_number", 1);
                page.width = pj.value("width", 1700);
                page.height = pj.value("height", 2200);

                if (pj.contains("blocks") && pj["blocks"].is_array()) {
                    for (const auto& bj : pj["blocks"]) {
                        representation::RepresentationBlock b;
                        b.id = bj.value("id", "");
                        b.text = bj.value("text", "");
                        b.label = bj.value("label", "");
                        b.value = bj.value("value", "");

                        std::string tStr = bj.value("type", "paragraph");
                        if (tStr == "header") b.type = representation::BlockType::Header;
                        else if (tStr == "footer") b.type = representation::BlockType::Footer;
                        else if (tStr == "heading") b.type = representation::BlockType::Heading;
                        else if (tStr == "label_value") b.type = representation::BlockType::LabelValue;
                        else b.type = representation::BlockType::Paragraph;

                        if (bj.contains("source") && bj["source"].contains("bbox")) {
                            const auto& bboxJ = bj["source"]["bbox"];
                            b.source.bbox.x = bboxJ.value("x", 0);
                            b.source.bbox.y = bboxJ.value("y", 0);
                            b.source.bbox.width = bboxJ.value("width", 0);
                            b.source.bbox.height = bboxJ.value("height", 0);
                            b.source.pageNumber = page.pageNumber;
                        }

                        if (bj.contains("confidence")) {
                            const auto& cj = bj["confidence"];
                            b.confidence.mean = cj.value("mean", 1.0f);
                            b.confidence.minimum = cj.value("minimum", 1.0f);
                            b.confidence.maximum = cj.value("maximum", 1.0f);
                            b.confidence.tokenCount = cj.value("token_count", 1);
                            b.confidence.lowConfidenceTokenCount = cj.value("low_confidence_token_count", 0);
                        }

                        page.blocks.push_back(std::move(b));
                    }
                }
                doc.pages.push_back(std::move(page));
            }
        }

        if (fullEdrJson.contains("tables") && fullEdrJson["tables"].is_array()) {
            for (const auto& tj : fullEdrJson["tables"]) {
                representation::SemanticTable tbl;
                tbl.id = tj.value("id", "");
                tbl.caption = tj.value("title", tj.value("caption", ""));
                tbl.tableType = tj.value("table_type", "course_records");
                tbl.pageStart = tj.value("page_start", 1);
                tbl.pageEnd = tj.value("page_end", 1);

                if (tj.contains("columns") && tj["columns"].is_array()) {
                    for (const auto& colJ : tj["columns"]) {
                        representation::SemanticColumn c;
                        c.name = colJ.value("name", "");
                        c.canonicalName = colJ.value("canonical_name", "");
                        c.columnIndex = colJ.value("column_index", 0);
                        tbl.columns.push_back(std::move(c));
                    }
                }

                if (tj.contains("rows") && tj["rows"].is_array()) {
                    for (const auto& rowJ : tj["rows"]) {
                        representation::SemanticRow r;
                        r.confidence = rowJ.value("confidence", 1.0f);

                        if (rowJ.contains("fields") && rowJ["fields"].is_object()) {
                            for (auto it = rowJ["fields"].begin(); it != rowJ["fields"].end(); ++it) {
                                if (it.value().is_string()) {
                                    r.canonicalFields[it.key()] = it.value().get<std::string>();
                                }
                            }
                        } else {
                            for (auto it = rowJ.begin(); it != rowJ.end(); ++it) {
                                if (it.value().is_string() && it.key() != "confidence") {
                                    r.canonicalFields[it.key()] = it.value().get<std::string>();
                                }
                            }
                        }

                        if (rowJ.contains("cells") && rowJ["cells"].is_array()) {
                            for (const auto& cellJ : rowJ["cells"]) {
                                representation::SemanticCell cell;
                                cell.rawText = cellJ.value("text", "");
                                cell.confidence = cellJ.value("confidence", 1.0f);
                                r.cells.push_back(std::move(cell));
                            }
                        }

                        // If cells was empty but canonicalFields was populated, create cells from canonicalFields
                        if (r.cells.empty() && !r.canonicalFields.empty()) {
                            for (const auto& [k, v] : r.canonicalFields) {
                                representation::SemanticCell cell;
                                cell.rawText = v;
                                cell.confidence = r.confidence;
                                r.cells.push_back(std::move(cell));
                            }
                        }

                        tbl.rows.push_back(std::move(r));
                    }
                }
                doc.tables.push_back(std::move(tbl));
            }
        }

        if (fullEdrJson.contains("sections") && fullEdrJson["sections"].is_array()) {
            for (const auto& sj : fullEdrJson["sections"]) {
                representation::SemanticSection s;
                s.id = sj.value("id", "");
                s.title = sj.value("title", "");
                s.pageStart = sj.value("page_start", 1);
                s.pageEnd = sj.value("page_end", 1);

                std::string typeStr = sj.value("type", "unknown");
                s.type = representation::stringToSectionType(typeStr);
                if (s.type == representation::SectionType::Unknown && !s.id.empty()) {
                    std::string fallback = s.id;
                    if (fallback.rfind("section_", 0) == 0) fallback = fallback.substr(8);
                    else if (fallback.rfind("sec_", 0) == 0) fallback = fallback.substr(4);
                    s.type = representation::stringToSectionType(fallback);
                }

                if (sj.contains("fields") && sj["fields"].is_object()) {
                    for (auto it = sj["fields"].begin(); it != sj["fields"].end(); ++it) {
                        if (it.value().is_string()) {
                            s.fields[it.key()] = it.value().get<std::string>();
                        }
                    }
                }
                doc.sections.push_back(std::move(s));
            }
        }

        // Store Full EDR byte metrics
        std::string edrDump = fullEdrJson.dump();
        doc.compression.edrFullBytes = edrDump.size();
        doc.compression.edrFullCharacters = edrDump.size();
        doc.compression.estimatedFullTokens = edrDump.size() / 4;

        return pimpl_->build(doc, optionalRawOcrJson);
    } catch (const std::exception& e) {
        return Result<LlmContextBuildResult>::failure(
            ErrorCode::InvalidJson,
            std::string("Failed to parse Full EDR JSON for Context Builder: ") + e.what()
        );
    }
}

Result<LlmContextBuildResult> LlmContextBuilder::build(
    const std::filesystem::path& fullEdrJsonPath,
    const std::filesystem::path* optionalRawOcrJsonPath
) const {
    if (!std::filesystem::exists(fullEdrJsonPath)) {
        return Result<LlmContextBuildResult>::failure(
            ErrorCode::FileNotFound,
            "Full EDR JSON file not found: " + fullEdrJsonPath.string(),
            fullEdrJsonPath.string()
        );
    }

    std::ifstream edrFile(fullEdrJsonPath);
    if (!edrFile.is_open() || !edrFile.good()) {
        return Result<LlmContextBuildResult>::failure(
            ErrorCode::FileReadError,
            "Failed to open Full EDR JSON file: " + fullEdrJsonPath.string(),
            fullEdrJsonPath.string()
        );
    }

    try {
        nlohmann::json edrJson;
        edrFile >> edrJson;

        if (optionalRawOcrJsonPath != nullptr && std::filesystem::exists(*optionalRawOcrJsonPath)) {
            std::ifstream rawFile(*optionalRawOcrJsonPath);
            nlohmann::json rawJson;
            rawFile >> rawJson;
            return build(edrJson, &rawJson);
        }

        return build(edrJson, nullptr);
    } catch (const std::exception& e) {
        return Result<LlmContextBuildResult>::failure(
            ErrorCode::InvalidJson,
            std::string("JSON parsing error: ") + e.what(),
            fullEdrJsonPath.string()
        );
    }
}

bool LlmContextBuilder::saveToDirectory(
    const LlmContextBuildResult& result,
    const std::filesystem::path& outputDir,
    const std::string& fileStem,
    bool prettyPrint
) {
    (void)fileStem;
    std::filesystem::path contextPath = outputDir / "llm_context.json";
    std::filesystem::path reportPath = outputDir / "llm_context_report.json";

    bool ok1 = LlmContextSerializer::saveToFile(contextPath, result.context.toJson(false), prettyPrint);
    bool ok2 = LlmContextSerializer::saveToFile(reportPath, result.report.toJson(), prettyPrint);

    return ok1 && ok2;
}

} // namespace edmo::ocr::context
