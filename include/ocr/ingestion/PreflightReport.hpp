#pragma once

#include <edmo/ocr/ingestion/IngestionTypes.hpp>

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace edmo::ocr::ingestion {

/**
 * @brief Serializer and formatter for preflight reports.
 */
class PreflightReport {
public:
    /**
     * @brief Convert summary to JSON object.
     */
    [[nodiscard]] static nlohmann::json toJson(const PreflightSummary& summary);

    /**
     * @brief Serialize summary to JSON string.
     */
    [[nodiscard]] static std::string serialize(const PreflightSummary& summary, bool prettyPrint = true);

    /**
     * @brief Save preflight report to file.
     */
    [[nodiscard]] static bool saveToFile(
        const std::filesystem::path& outputPath,
        const PreflightSummary& summary,
        bool prettyPrint = true
    );
};

} // namespace edmo::ocr::ingestion
