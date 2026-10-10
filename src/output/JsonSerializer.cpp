#include "JsonSerializer.hpp"
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"

#include <fstream>

namespace edmo::ocr {

std::string JsonSerializer::serialize(
    const OcrResult& result,
    OutputFormat format,
    bool prettyPrint
) {
    if (format == OutputFormat::PlainText) {
        return result.toPlainText();
    }

    nlohmann::json j = result.toJson(format);
    return prettyPrint ? j.dump(2) : j.dump();
}

Result<void> JsonSerializer::saveToFile(
    const std::filesystem::path& outputPath,
    const OcrResult& result,
    OutputFormat format,
    bool prettyPrint
) {
    std::error_code ec;
    auto parent = outputPath.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
        std::filesystem::create_directories(parent, ec);
    }

    std::ofstream out(outputPath);
    if (!out.is_open() || !out.good()) {
        return Result<void>::failure(
            ErrorCode::SerializationFailed,
            "Failed to open output file for writing",
            outputPath.string()
        );
    }

    out << serialize(result, format, prettyPrint);
    if (!out.good()) {
        return Result<void>::failure(
            ErrorCode::SerializationFailed,
            "Error writing serialization data to output stream",
            outputPath.string()
        );
    }

    return Result<void>::success();
}

} // namespace edmo::ocr
