#pragma once

#include <edmo/ocr/OcrResult.hpp>
#include <edmo/ocr/Result.hpp>
#include <edmo/ocr/Types.hpp>

#include <filesystem>
#include <string>

namespace edmo::ocr {

class JsonSerializer {
public:
    static std::string serialize(
        const OcrResult& result,
        OutputFormat format = OutputFormat::FullJson,
        bool prettyPrint = true
    );

    static Result<void> saveToFile(
        const std::filesystem::path& outputPath,
        const OcrResult& result,
        OutputFormat format = OutputFormat::FullJson,
        bool prettyPrint = true
    );
};

} // namespace edmo::ocr
