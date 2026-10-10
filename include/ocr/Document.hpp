#pragma once

#include <edmo/ocr/Page.hpp>
#include <edmo/ocr/Result.hpp>
#include <edmo/ocr/Types.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace edmo::ocr {

/**
 * @brief Represents a loaded document containing one or more pages.
 */
struct Document {
    std::string id;
    std::string sourcePath;
    DocumentType type = DocumentType::Unknown;
    size_t fileSizeBytes = 0;
    std::string mimeType;
    std::vector<Page> pages;

    [[nodiscard]] size_t pageCount() const noexcept {
        return pages.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return pages.empty();
    }

    /**
     * @brief Factory method to inspect and load an input document safely.
     */
    static Result<Document> load(
        const std::filesystem::path& filePath,
        size_t maxFileSizeMb = 50,
        size_t maxPages = 100
    );
};

} // namespace edmo::ocr
