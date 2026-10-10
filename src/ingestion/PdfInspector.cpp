#include <edmo/ocr/ingestion/PdfInspector.hpp>
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/ImageUtils.hpp"
#include "../utils/Timer.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <poppler-document.h>
#include <poppler-page.h>

namespace edmo::ocr::ingestion {

PdfInspector::PdfInspector(IngestionConfig config)
    : config_(config) {}

Result<void> PdfInspector::validate(const std::filesystem::path& documentPath) const {
    std::error_code ec;
    if (!std::filesystem::exists(documentPath, ec)) {
        return Result<void>::failure(
            ErrorCode::FileNotFound,
            "File does not exist: " + documentPath.string(),
            documentPath.string()
        );
    }

    if (!std::filesystem::is_regular_file(documentPath, ec)) {
        return Result<void>::failure(
            ErrorCode::InvalidDocument,
            "Path is not a regular file: " + documentPath.string(),
            documentPath.string()
        );
    }

    auto fileSize = std::filesystem::file_size(documentPath, ec);
    if (ec || fileSize == 0) {
        return Result<void>::failure(
            ErrorCode::FileEmpty,
            "File is empty (0 bytes): " + documentPath.string(),
            documentPath.string()
        );
    }

    size_t maxBytes = config_.maxFileSizeMb * 1024 * 1024;
    if (fileSize > maxBytes) {
        return Result<void>::failure(
            ErrorCode::FileTooLarge,
            "File size (" + std::to_string(fileSize / (1024 * 1024)) + " MB) exceeds configured limit (" +
                std::to_string(config_.maxFileSizeMb) + " MB)",
            documentPath.string()
        );
    }

    std::string ext = documentPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

    if (ext != ".pdf" && ext != ".png" && ext != ".jpg" && ext != ".jpeg" &&
        ext != ".tiff" && ext != ".tif" && ext != ".bmp" && ext != ".webp") {
        return Result<void>::failure(
            ErrorCode::UnsupportedFormat,
            "Unsupported file format extension: " + ext,
            documentPath.string()
        );
    }

    return Result<void>::success();
}

Result<PreflightSummary> PdfInspector::inspect(const std::filesystem::path& documentPath) const {
    auto valRes = validate(documentPath);
    if (!valRes) {
        return Result<PreflightSummary>::failure(valRes.error());
    }

    std::string ext = documentPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

    if (ext == ".pdf") {
        return inspectPdf(documentPath);
    } else {
        return inspectImage(documentPath);
    }
}

Result<PreflightSummary> PdfInspector::inspectPdf(const std::filesystem::path& pdfPath) const {
    Timer timer;
    PreflightSummary summary;
    summary.fileName = pdfPath.filename().string();
    summary.format = "PDF";
    summary.fileSizeBytes = FileUtils::getFileSize(pdfPath);

    // Fast header check
    std::ifstream file(pdfPath, std::ios::binary);
    if (!file.is_open()) {
        return Result<PreflightSummary>::failure(
            ErrorCode::InvalidDocument,
            "Unable to open PDF file: " + pdfPath.string(),
            pdfPath.string()
        );
    }
    char header[5] = {0};
    file.read(header, 4);
    if (std::string(header, 4) != "%PDF") {
        return Result<PreflightSummary>::failure(
            ErrorCode::InvalidDocument,
            "Invalid PDF header format: " + pdfPath.string(),
            pdfPath.string()
        );
    }
    file.close();

    // Poppler load for cheap metadata inspection
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdfPath.string()));
    if (!doc) {
        return Result<PreflightSummary>::failure(
            ErrorCode::PdfRenderingFailed,
            "Failed to parse PDF document (corrupted or malformed)",
            pdfPath.string()
        );
    }

    if (doc->is_locked()) {
        return Result<PreflightSummary>::failure(
            ErrorCode::PdfRenderingFailed,
            "PDF document is encrypted or password-protected",
            pdfPath.string()
        );
    }

    int totalPages = doc->pages();
    if (totalPages <= 0) {
        return Result<PreflightSummary>::failure(
            ErrorCode::PdfRenderingFailed,
            "PDF document contains 0 pages",
            pdfPath.string()
        );
    }

    if (totalPages > config_.maxPages) {
        return Result<PreflightSummary>::failure(
            ErrorCode::FileTooLarge,
            "PDF page count (" + std::to_string(totalPages) + ") exceeds configured limit (" +
                std::to_string(config_.maxPages) + ")",
            pdfPath.string()
        );
    }

    summary.pageCount = totalPages;
    summary.pages.reserve(totalPages);

    for (int i = 0; i < totalPages; ++i) {
        Timer pageTimer;
        int pageNum = i + 1;
        std::unique_ptr<poppler::page> p(doc->create_page(i));
        if (!p) {
            PageInspection failedPage;
            failedPage.pageNumber = pageNum;
            failedPage.inputType = PageInputType::Corrupted;
            failedPage.strategy = ProcessingStrategyType::Error;
            summary.pages.push_back(std::move(failedPage));
            continue;
        }

        poppler::rectf pageRect = p->page_rect();

        PageInspection pi;
        pi.pageNumber = pageNum;
        pi.width = static_cast<int>(pageRect.width());
        pi.height = static_cast<int>(pageRect.height());

        switch (p->orientation()) {
            case poppler::page::landscape:  pi.rotation = 90; break;
            case poppler::page::upside_down: pi.rotation = 180; break;
            case poppler::page::seascape:   pi.rotation = 270; break;
            default:                        pi.rotation = 0; break;
        }

        double pageArea = (pageRect.width() > 0 && pageRect.height() > 0)
            ? (pageRect.width() * pageRect.height())
            : 1.0;
        pi.textLayer.pageArea = pageArea;
        pi.images.pageArea = pageArea;

        // Text layer inspection
        std::vector<poppler::text_box> textBoxes = p->text_list();
        int charCount = 0;
        int wordCount = static_cast<int>(textBoxes.size());
        int printableCount = 0;
        int alphaCount = 0;
        int whitespaceCount = 0;
        double textBoundingArea = 0.0;

        for (const auto& box : textBoxes) {
            poppler::ustring ustr = box.text();
            poppler::rectf bbox = box.bbox();
            textBoundingArea += (bbox.width() * bbox.height());

            for (size_t c = 0; c < ustr.length(); ++c) {
                char ch = static_cast<char>(ustr[c]);
                charCount++;
                if (std::isprint(static_cast<unsigned char>(ch))) printableCount++;
                if (std::isalpha(static_cast<unsigned char>(ch))) alphaCount++;
                if (std::isspace(static_cast<unsigned char>(ch))) whitespaceCount++;
            }
        }

        pi.textLayer.exists = (charCount > 0);
        pi.textLayer.characterCount = charCount;
        pi.textLayer.wordCount = wordCount;
        pi.textLayer.textBoundingBoxArea = textBoundingArea;
        pi.textLayer.textDensity = std::clamp(textBoundingArea / pageArea, 0.0, 1.0);

        if (charCount > 0) {
            pi.textLayer.printableCharRatio = static_cast<double>(printableCount) / charCount;
            pi.textLayer.alphabeticRatio = static_cast<double>(alphaCount) / charCount;
            pi.textLayer.whitespaceRatio = static_cast<double>(whitespaceCount) / charCount;
            pi.textLayer.avgWordLength = wordCount > 0 ? (static_cast<double>(charCount) / wordCount) : 0.0;

            double charScore = std::min(1.0, static_cast<double>(charCount) / 400.0);
            double wordScore = std::min(1.0, static_cast<double>(wordCount) / 60.0);
            pi.textLayer.qualityScore = std::clamp(
                (0.40 * charScore) +
                (0.30 * pi.textLayer.printableCharRatio) +
                (0.20 * wordScore) +
                (0.10 * pi.textLayer.alphabeticRatio),
                0.0, 1.0
            );

            // Native text spatial reliability analysis (detect overlapping/ghost text collisions)
            NativeTextReliability reliability;
            reliability.isReliable = true;
            reliability.suspiciousOverlapCount = 0;
            reliability.overlapRatio = 0.0;

            if (textBoxes.size() >= 2) {
                struct BoxData {
                    std::string text;
                    poppler::rectf bbox;
                    double area;
                };

                std::vector<BoxData> boxes;
                boxes.reserve(textBoxes.size());
                for (const auto& b : textBoxes) {
                    poppler::ustring ustr = b.text();
                    if (ustr.empty()) continue;
                    std::string s;
                    for (size_t c = 0; c < ustr.length(); ++c) s.push_back(static_cast<char>(ustr[c]));
                    poppler::rectf rect = b.bbox();
                    double area = rect.width() * rect.height();
                    if (area > 0.0) {
                        boxes.push_back({std::move(s), rect, area});
                    }
                }

                int collisionCount = 0;
                for (size_t i = 0; i < boxes.size(); ++i) {
                    for (size_t j = i + 1; j < boxes.size(); ++j) {
                        double x1 = std::max(boxes[i].bbox.x(), boxes[j].bbox.x());
                        double y1 = std::max(boxes[i].bbox.y(), boxes[j].bbox.y());
                        double x2 = std::min(boxes[i].bbox.x() + boxes[i].bbox.width(), boxes[j].bbox.x() + boxes[j].bbox.width());
                        double y2 = std::min(boxes[i].bbox.y() + boxes[i].bbox.height(), boxes[j].bbox.y() + boxes[j].bbox.height());

                        if (x2 > x1 && y2 > y1) {
                            double interArea = (x2 - x1) * (y2 - y1);
                            double minArea = std::min(boxes[i].area, boxes[j].area);
                            if (minArea > 0.0) {
                                double overlapRatio = interArea / minArea;
                                if (overlapRatio >= 0.40 && boxes[i].text != boxes[j].text) {
                                    collisionCount++;
                                }
                            }
                        }
                    }
                }

                if (collisionCount > 0) {
                    reliability.isReliable = false;
                    reliability.suspiciousOverlapCount = collisionCount;
                    reliability.overlapRatio = boxes.empty() ? 0.0 : (static_cast<double>(collisionCount) / boxes.size());
                    reliability.triggerReason = "NATIVE_TEXT_SPATIAL_COLLISION_DETECTED";
                }
            }
            pi.textLayer.reliability = reliability;

            pi.textLayer.meaningful = (
                charCount >= config_.minMeaningfulCharCount &&
                wordCount >= config_.minMeaningfulWordCount &&
                pi.textLayer.printableCharRatio >= config_.minPrintableRatio &&
                pi.textLayer.qualityScore >= config_.minQualityScore &&
                pi.textLayer.reliability.isReliable
            );
        } else {
            pi.textLayer.meaningful = false;
            pi.textLayer.qualityScore = 0.0;
        }

        // Image coverage estimation
        if (!pi.textLayer.meaningful) {
            // In a scanned PDF without text layer, the scanned page is an image covering the canvas
            pi.images.count = 1;
            pi.images.coverageRatio = 0.98;
            pi.images.largestImageRatio = 0.98;
        } else {
            pi.images.count = 0;
            pi.images.coverageRatio = 0.0;
            pi.images.largestImageRatio = 0.0;
        }

        pi.timing.preflightMs = pageTimer.elapsedMs();
        summary.pages.push_back(std::move(pi));
    }

    summary.performance.preflightTimeMs = timer.elapsedMs();
    return Result<PreflightSummary>::success(std::move(summary));
}

Result<PreflightSummary> PdfInspector::inspectImage(const std::filesystem::path& imagePath) const {
    Timer timer;
    PreflightSummary summary;
    summary.fileName = imagePath.filename().string();
    summary.format = "IMAGE";
    summary.fileSizeBytes = FileUtils::getFileSize(imagePath);
    summary.pageCount = 1;

    PageInspection pi;
    pi.pageNumber = 1;
    pi.inputType = PageInputType::ScannedImage;
    pi.strategy = ProcessingStrategyType::Ocr;

    pi.textLayer.exists = false;
    pi.textLayer.meaningful = false;
    pi.textLayer.characterCount = 0;
    pi.textLayer.wordCount = 0;
    pi.textLayer.qualityScore = 0.0;

    pi.images.count = 1;
    pi.images.coverageRatio = 1.0;
    pi.images.largestImageRatio = 1.0;

    pi.timing.preflightMs = timer.elapsedMs();
    summary.pages.push_back(std::move(pi));

    summary.documentType = DocumentLevelType::ScannedImage;
    summary.performance.preflightTimeMs = timer.elapsedMs();
    return Result<PreflightSummary>::success(std::move(summary));
}

} // namespace edmo::ocr::ingestion
