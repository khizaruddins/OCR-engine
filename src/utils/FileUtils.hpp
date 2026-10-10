#pragma once

#include <ocr/Result.hpp>
#include <ocr/Types.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace ocr {

class FileUtils {
public:
    static Result<void> validateFile(
        const std::filesystem::path& path,
        size_t maxFileSizeMb = 50
    );

    static DocumentType detectDocumentType(const std::filesystem::path& path);
    static std::string getMimeType(const std::filesystem::path& path);
    static size_t getFileSize(const std::filesystem::path& path);
    static bool isSupportedExtension(const std::filesystem::path& path);
    static std::vector<std::filesystem::path> listSupportedFiles(
        const std::filesystem::path& directoryPath,
        bool recursive = false
    );

    static std::string generateDocumentId();
    static std::string generateProcessingId();
};

} // namespace ocr
