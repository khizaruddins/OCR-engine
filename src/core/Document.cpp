#include <ocr/Document.hpp>
#include <ocr/ErrorCode.hpp>
#include "../utils/FileUtils.hpp"
#include "../utils/ImageUtils.hpp"

namespace ocr {

Result<Document> Document::load(
    const std::filesystem::path& filePath,
    size_t maxFileSizeMb,
    size_t maxPages
) {
    (void)maxPages; // Validated during PDF rendering
    auto validRes = FileUtils::validateFile(filePath, maxFileSizeMb);
    if (!validRes) {
        return Result<Document>::failure(validRes.error());
    }

    Document doc;
    doc.id = FileUtils::generateDocumentId();
    doc.sourcePath = filePath.string();
    doc.type = FileUtils::detectDocumentType(filePath);
    doc.fileSizeBytes = FileUtils::getFileSize(filePath);
    doc.mimeType = FileUtils::getMimeType(filePath);

    if (doc.type == DocumentType::Image) {
        auto imgRes = ImageUtils::loadMat(filePath);
        if (!imgRes) {
            return Result<Document>::failure(imgRes.error());
        }

        Page page;
        page.pageNumber = 1;
        page.width = imgRes.value().cols;
        page.height = imgRes.value().rows;
        page.dpi = 300;
        page.image = Image(imgRes.value());

        doc.pages.push_back(std::move(page));
    }

    return Result<Document>::success(std::move(doc));
}

} // namespace ocr
