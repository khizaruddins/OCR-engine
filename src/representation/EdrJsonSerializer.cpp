#include "EdrJsonSerializer.hpp"

#include <fstream>

namespace edmo::ocr::representation {

nlohmann::json EdrJsonSerializer::toFullJson(const SemanticDocument& doc) {
    int nativePagesCount = 0;
    int ocrPagesCount = 0;
    int skippedPagesCount = 0;
    int failedPagesCount = 0;

    for (const auto& p : doc.pages) {
        if (p.processingStrategy == "NATIVE_TEXT") nativePagesCount++;
        else if (p.processingStrategy == "SKIP_EMPTY") skippedPagesCount++;
        else if (p.processingStrategy == "ERROR") failedPagesCount++;
        else ocrPagesCount++;
    }

    nlohmann::json j{
        {"schema_version", doc.schemaVersion},
        {"document", {
            {"id", doc.metadata.id},
            {"type", doc.documentType},
            {"page_count", doc.metadata.pageCount},
            {"source", doc.metadata.source}
        }},
        {"engine", doc.engine.toJson()},
        {"processing_manifest", {
            {"engine", "edmo-ocr"},
            {"version", "0.2.0"},
            {"document", {
                {"type", doc.documentClassification.empty() ? doc.documentType : doc.documentClassification},
                {"pages", doc.metadata.pageCount}
            }},
            {"strategy", doc.strategy.empty() ? "ADAPTIVE" : doc.strategy},
            {"pages", {
                {"native_text", nativePagesCount},
                {"ocr", ocrPagesCount},
                {"ocr_enhanced", 0},
                {"skipped", skippedPagesCount},
                {"failed", failedPagesCount}
            }}
        }},
        {"confidence", doc.confidence.toJson()}
    };

    if (!doc.identity.empty()) {
        nlohmann::json idJson = nlohmann::json::object();
        for (const auto& [k, v] : doc.identity) {
            idJson[k] = v;
        }
        j["identity"] = std::move(idJson);
    }

    if (!doc.institution.empty()) {
        nlohmann::json instJson = nlohmann::json::object();
        for (const auto& [k, v] : doc.institution) {
            instJson[k] = v;
        }
        j["institution"] = std::move(instJson);
    }

    if (!doc.academic.empty()) {
        nlohmann::json acadJson = nlohmann::json::object();
        for (const auto& [k, v] : doc.academic) {
            acadJson[k] = v;
        }
        j["academic"] = std::move(acadJson);
    }

    // Pages
    nlohmann::json pagesJson = nlohmann::json::array();
    for (const auto& p : doc.pages) {
        pagesJson.push_back(p.toJson(true));
    }
    j["pages"] = std::move(pagesJson);

    // Sections
    nlohmann::json sectionsJson = nlohmann::json::array();
    for (const auto& s : doc.sections) {
        sectionsJson.push_back(s.toJson(true));
    }
    j["sections"] = std::move(sectionsJson);

    // Tables
    nlohmann::json tablesJson = nlohmann::json::array();
    for (const auto& t : doc.tables) {
        tablesJson.push_back(t.toJson(true));
    }
    j["tables"] = std::move(tablesJson);

    if (!doc.warnings.empty()) {
        nlohmann::json warnJson = nlohmann::json::array();
        for (const auto& w : doc.warnings) {
            warnJson.push_back(w.toJson());
        }
        j["warnings"] = std::move(warnJson);
    }

    if (doc.compression.rawBytes > 0) {
        j["compression"] = doc.compression.toJson();
    }

    return j;
}

nlohmann::json EdrJsonSerializer::toLlmJson(
    const SemanticDocument& doc,
    bool includeSource
) {
    nlohmann::json j{
        {"document_type", doc.documentType},
        {"page_count", doc.metadata.pageCount}
    };

    // Student identity
    if (!doc.identity.empty()) {
        nlohmann::json studentJson = nlohmann::json::object();
        for (const auto& [k, v] : doc.identity) {
            studentJson[k] = v;
        }
        j["student"] = std::move(studentJson);
    }

    // Institution
    if (!doc.institution.empty()) {
        nlohmann::json instJson = nlohmann::json::object();
        for (const auto& [k, v] : doc.institution) {
            instJson[k] = v;
        }
        j["institution"] = std::move(instJson);
    }

    // Academic
    if (!doc.academic.empty()) {
        nlohmann::json acadJson = nlohmann::json::object();
        for (const auto& [k, v] : doc.academic) {
            acadJson[k] = v;
        }
        j["academic"] = std::move(acadJson);
    }

    // Academic records (course tables)
    nlohmann::json recordsJson = nlohmann::json::array();
    for (const auto& tbl : doc.tables) {
        for (const auto& row : tbl.rows) {
            if (row.isHeader) continue;

            nlohmann::json rowObj = nlohmann::json::object();
            if (!row.canonicalFields.empty()) {
                for (const auto& [k, v] : row.canonicalFields) {
                    rowObj[k] = v;
                }
            } else {
                for (size_t c = 0; c < row.cells.size(); ++c) {
                    std::string colName = c < tbl.columns.size() ? tbl.columns[c].canonicalName : ("col_" + std::to_string(c));
                    rowObj[colName] = row.cells[c].text();
                }
            }

            if (includeSource && !row.source.empty()) {
                rowObj["source"] = row.source.toJson(false);
            }

            if (!rowObj.empty()) {
                recordsJson.push_back(std::move(rowObj));
            }
        }
    }

    if (!recordsJson.empty()) {
        j["academic_records"] = std::move(recordsJson);
    }

    // Unclassified content preservation
    if (!doc.unclassifiedContent.empty()) {
        j["unclassified_content"] = doc.unclassifiedContent;
    }

    return j;
}

std::string EdrJsonSerializer::serializeFull(
    const SemanticDocument& doc,
    bool prettyPrint
) {
    nlohmann::json j = toFullJson(doc);
    return prettyPrint ? j.dump(2) : j.dump();
}

std::string EdrJsonSerializer::serializeLlm(
    const SemanticDocument& doc,
    bool includeSource,
    bool prettyPrint
) {
    nlohmann::json j = toLlmJson(doc, includeSource);
    return prettyPrint ? j.dump(2) : j.dump();
}

bool EdrJsonSerializer::saveToFile(
    const std::filesystem::path& path,
    const nlohmann::json& json,
    bool prettyPrint
) {
    std::error_code ec;
    auto parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
        std::filesystem::create_directories(parent, ec);
    }

    std::ofstream out(path);
    if (!out.is_open() || !out.good()) {
        return false;
    }

    out << (prettyPrint ? json.dump(2) : json.dump());
    return out.good();
}

} // namespace edmo::ocr::representation
