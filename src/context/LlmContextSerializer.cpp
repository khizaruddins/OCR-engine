#include "LlmContextSerializer.hpp"

#include <fstream>
#include <iomanip>

namespace edmo::ocr::context {

nlohmann::json LlmContextSerializer::toJson(
    const LlmContext& context,
    ContextMode mode
) {
    return context.toJson(mode == ContextMode::Traceable);
}

nlohmann::json LlmContextSerializer::toReportJson(
    const LlmContextReport& report
) {
    return report.toJson();
}

bool LlmContextSerializer::saveToFile(
    const std::filesystem::path& path,
    const nlohmann::json& json,
    bool prettyPrint
) {
    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    std::ofstream file(path);
    if (!file.is_open() || !file.good()) {
        return false;
    }

    if (prettyPrint) {
        file << std::setw(2) << json << std::endl;
    } else {
        file << json.dump() << std::endl;
    }

    return file.good();
}

} // namespace edmo::ocr::context
