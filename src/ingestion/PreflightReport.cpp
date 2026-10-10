#include <ocr/ingestion/PreflightReport.hpp>

#include <fstream>

namespace ocr::ingestion {

nlohmann::json PreflightReport::toJson(const PreflightSummary& summary) {
    return summary.toJson();
}

std::string PreflightReport::serialize(const PreflightSummary& summary, bool prettyPrint) {
    nlohmann::json j = toJson(summary);
    return prettyPrint ? j.dump(2) : j.dump();
}

bool PreflightReport::saveToFile(
    const std::filesystem::path& outputPath,
    const PreflightSummary& summary,
    bool prettyPrint
) {
    std::error_code ec;
    auto parent = outputPath.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
        std::filesystem::create_directories(parent, ec);
    }

    std::ofstream out(outputPath);
    if (!out.is_open() || !out.good()) {
        return false;
    }

    out << serialize(summary, prettyPrint);
    return out.good();
}

} // namespace ocr::ingestion
