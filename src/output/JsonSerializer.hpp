#pragma once

#include <ocr/OcrResult.hpp>
#include <ocr/Result.hpp>
#include <ocr/Types.hpp>

#include <filesystem>
#include <string>

namespace ocr {

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

} // namespace ocr
