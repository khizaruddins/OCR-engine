#include "PdfRenderer.hpp"
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"
#include "../utils/ImageUtils.hpp"

#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

namespace edmo::ocr {

Result<int> PdfRenderer::getPageCount(const std::filesystem::path& pdfPath) {
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdfPath.string()));
    if (!doc) {
        return Result<int>::failure(
            ErrorCode::PdfRenderingFailed,
            "Failed to open PDF document (corrupted or encrypted)",
            pdfPath.string()
        );
    }

    if (doc->is_locked()) {
        return Result<int>::failure(
            ErrorCode::PdfRenderingFailed,
            "PDF document is password protected / locked",
            pdfPath.string()
        );
    }

    return Result<int>::success(doc->pages());
}

Result<std::vector<Page>> PdfRenderer::render(
    const std::filesystem::path& pdfPath,
    const PdfConfig& config
) {
    EDMO_LOG_INFO("Loading PDF document: {}", pdfPath.string());

    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdfPath.string()));
    if (!doc) {
        return Result<std::vector<Page>>::failure(
            ErrorCode::PdfRenderingFailed,
            "Failed to parse PDF document (invalid or corrupt file)",
            pdfPath.string()
        );
    }

    if (doc->is_locked()) {
        return Result<std::vector<Page>>::failure(
            ErrorCode::PdfRenderingFailed,
            "PDF document is password-protected",
            pdfPath.string()
        );
    }

    int totalPages = doc->pages();
    EDMO_LOG_INFO("Detected PDF with {} page(s)", totalPages);

    if (totalPages <= 0) {
        return Result<std::vector<Page>>::failure(
            ErrorCode::PdfRenderingFailed,
            "PDF contains 0 pages",
            pdfPath.string()
        );
    }

    if (totalPages > config.maxPages) {
        return Result<std::vector<Page>>::failure(
            ErrorCode::FileTooLarge,
            "PDF page count (" + std::to_string(totalPages) + ") exceeds configured maximum limit (" +
                std::to_string(config.maxPages) + ")",
            pdfPath.string()
        );
    }

    poppler::page_renderer renderer;
    renderer.set_render_hint(poppler::page_renderer::antialiasing, true);
    renderer.set_render_hint(poppler::page_renderer::text_antialiasing, true);

    std::vector<Page> pages;
    pages.reserve(totalPages);

    for (int i = 0; i < totalPages; ++i) {
        int pageNum = i + 1;
        EDMO_LOG_DEBUG("Rendering PDF page {}/{} at {} DPI", pageNum, totalPages, config.dpi);

        std::unique_ptr<poppler::page> p(doc->create_page(i));
        if (!p) {
            return Result<std::vector<Page>>::failure(
                ErrorCode::PdfRenderingFailed,
                "Failed to access page " + std::to_string(pageNum),
                pdfPath.string(),
                pageNum
            );
        }

        poppler::image renderedImg = renderer.render_page(p.get(), config.dpi, config.dpi);
        if (!renderedImg.is_valid()) {
            return Result<std::vector<Page>>::failure(
                ErrorCode::PdfRenderingFailed,
                "Failed to render page " + std::to_string(pageNum) + " at " + std::to_string(config.dpi) + " DPI",
                pdfPath.string(),
                pageNum
            );
        }

        auto matRes = ImageUtils::popplerImageToMat(renderedImg);
        if (!matRes) {
            return Result<std::vector<Page>>::failure(
                matRes.error().code,
                "Failed to convert rendered image on page " + std::to_string(pageNum) + ": " + matRes.error().message,
                pdfPath.string(),
                pageNum
            );
        }

        cv::Mat mat = matRes.value();

        Page page;
        page.pageNumber = pageNum;
        page.width = mat.cols;
        page.height = mat.rows;
        page.dpi = config.dpi;
        page.image = Image(std::move(mat));

        pages.push_back(std::move(page));
    }

    return Result<std::vector<Page>>::success(std::move(pages));
}

} // namespace edmo::ocr
