#include <ocr/ingestion/NativeTextExtractor.hpp>
#include <ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"
#include "../utils/Timer.hpp"

#include <algorithm>
#include <cmath>
#include <poppler-document.h>
#include <poppler-page.h>

namespace ocr::ingestion {

NativeTextExtractor::NativeTextExtractor(IngestionConfig config)
    : config_(config) {}

Result<OcrPageResult> NativeTextExtractor::extractPage(
    const std::filesystem::path& pdfPath,
    int pageNumber,
    int targetDpi
) const {
    Timer timer;
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdfPath.string()));
    if (!doc) {
        return Result<OcrPageResult>::failure(
            ErrorCode::PdfRenderingFailed,
            "Failed to open PDF document for native extraction: " + pdfPath.string(),
            pdfPath.string(),
            pageNumber
        );
    }

    if (pageNumber < 1 || pageNumber > doc->pages()) {
        return Result<OcrPageResult>::failure(
            ErrorCode::PdfRenderingFailed,
            "Page index out of bounds: " + std::to_string(pageNumber),
            pdfPath.string(),
            pageNumber
        );
    }

    std::unique_ptr<poppler::page> p(doc->create_page(pageNumber - 1));
    if (!p) {
        return Result<OcrPageResult>::failure(
            ErrorCode::PdfRenderingFailed,
            "Failed to access page " + std::to_string(pageNumber),
            pdfPath.string(),
            pageNumber
        );
    }

    poppler::rectf pageRect = p->page_rect();

    double scale = static_cast<double>(targetDpi) / 72.0;
    int pixelWidth = static_cast<int>(std::round(pageRect.width() * scale));
    int pixelHeight = static_cast<int>(std::round(pageRect.height() * scale));

    int rotation = 0;
    switch (p->orientation()) {
        case poppler::page::landscape:   rotation = 90; break;
        case poppler::page::upside_down: rotation = 180; break;
        case poppler::page::seascape:    rotation = 270; break;
        default:                         rotation = 0; break;
    }

    std::vector<poppler::text_box> textBoxes = p->text_list();
    if (textBoxes.empty()) {
        return Result<OcrPageResult>::failure(
            ErrorCode::PdfRenderingFailed,
            "No native text boxes found on page " + std::to_string(pageNumber),
            pdfPath.string(),
            pageNumber
        );
    }

    struct TempWord {
        std::string text;
        BoundingBox bbox;
        double yCenter;
    };

    std::vector<TempWord> words;
    words.reserve(textBoxes.size());

    for (const auto& box : textBoxes) {
        poppler::ustring ustr = box.text();
        if (ustr.empty()) continue;

        std::string wordStr;
        wordStr.reserve(ustr.length());
        for (size_t c = 0; c < ustr.length(); ++c) {
            wordStr.push_back(static_cast<char>(ustr[c]));
        }

        // Trim leading/trailing whitespace
        auto start = wordStr.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto end = wordStr.find_last_not_of(" \t\r\n");
        wordStr = wordStr.substr(start, end - start + 1);
        if (wordStr.empty()) continue;

        poppler::rectf bbox = box.bbox();
        int bx = static_cast<int>(std::round(bbox.x() * scale));
        int by = static_cast<int>(std::round(bbox.y() * scale));
        int bw = static_cast<int>(std::round(bbox.width() * scale));
        int bh = static_cast<int>(std::round(bbox.height() * scale));

        // Enforce valid positive dimensions
        if (bw <= 0) bw = 1;
        if (bh <= 0) bh = 1;

        BoundingBox wordBox(bx, by, bw, bh);
        double yCenter = by + (bh / 2.0);
        words.push_back({std::move(wordStr), wordBox, yCenter});
    }

    if (words.empty()) {
        return Result<OcrPageResult>::failure(
            ErrorCode::PdfRenderingFailed,
            "Native text extraction yielded empty text content on page " + std::to_string(pageNumber),
            pdfPath.string(),
            pageNumber
        );
    }

    // Sort words top-to-bottom
    std::sort(words.begin(), words.end(), [](const TempWord& a, const TempWord& b) {
        if (std::abs(a.bbox.y - b.bbox.y) < 6) {
            return a.bbox.x < b.bbox.x;
        }
        return a.bbox.y < b.bbox.y;
    });

    // Group words into lines
    struct TempLine {
        std::vector<TempWord> words;
        BoundingBox bbox;
    };

    std::vector<TempLine> lines;
    for (auto& w : words) {
        bool added = false;
        for (auto& l : lines) {
            int lineAvgH = l.bbox.height / std::max(1, static_cast<int>(l.words.size()));
            int tolerance = std::max(6, lineAvgH / 2);
            if (std::abs(w.yCenter - (l.bbox.y + l.bbox.height / 2.0)) <= tolerance) {
                l.words.push_back(w);
                l.bbox = l.bbox.unionWith(w.bbox);
                added = true;
                break;
            }
        }
        if (!added) {
            TempLine newLine;
            newLine.bbox = w.bbox;
            newLine.words.push_back(w);
            lines.push_back(std::move(newLine));
        }
    }

    // Sort words in each line horizontally
    for (auto& l : lines) {
        std::sort(l.words.begin(), l.words.end(), [](const TempWord& a, const TempWord& b) {
            return a.bbox.x < b.bbox.x;
        });
    }

    // Sort lines vertically
    std::sort(lines.begin(), lines.end(), [](const TempLine& a, const TempLine& b) {
        return a.bbox.y < b.bbox.y;
    });

    // Group lines into blocks
    struct TempBlock {
        std::vector<TempLine> lines;
        BoundingBox bbox;
    };

    std::vector<TempBlock> blocks;
    for (auto& l : lines) {
        if (blocks.empty()) {
            TempBlock tb;
            tb.bbox = l.bbox;
            tb.lines.push_back(std::move(l));
            blocks.push_back(std::move(tb));
        } else {
            auto& lastBlock = blocks.back();
            int verticalGap = l.bbox.y - (lastBlock.bbox.y + lastBlock.bbox.height);
            int avgLineH = std::max(10, l.bbox.height);
            if (verticalGap <= (avgLineH * 2)) {
                lastBlock.bbox = lastBlock.bbox.unionWith(l.bbox);
                lastBlock.lines.push_back(std::move(l));
            } else {
                TempBlock tb;
                tb.bbox = l.bbox;
                tb.lines.push_back(std::move(l));
                blocks.push_back(std::move(tb));
            }
        }
    }

    OcrPageResult pageResult;
    pageResult.pageNumber = pageNumber;
    pageResult.width = pixelWidth;
    pageResult.height = pixelHeight;
    pageResult.rotation = rotation;
    pageResult.skewAngle = 0.0;
    pageResult.confidence = 1.0f;

    std::string fullPageText;

    for (const auto& tb : blocks) {
        OcrBlock ob;
        ob.bbox = tb.bbox;
        ob.confidence = 1.0f;

        std::string blockText;

        for (const auto& tl : tb.lines) {
            OcrLine ol;
            ol.bbox = tl.bbox;
            ol.confidence = 1.0f;

            std::string lineText;

            for (const auto& tw : tl.words) {
                OcrToken ot;
                ot.text = tw.text;
                ot.bbox = tw.bbox;
                ot.confidence = 1.0f;

                if (!lineText.empty()) lineText += " ";
                lineText += tw.text;

                ol.tokens.push_back(std::move(ot));
            }

            ol.text = lineText;
            if (!blockText.empty()) blockText += "\n";
            blockText += lineText;

            ob.lines.push_back(std::move(ol));
        }

        ob.text = blockText;
        if (!fullPageText.empty()) fullPageText += "\n\n";
        fullPageText += blockText;

        pageResult.blocks.push_back(std::move(ob));
    }

    pageResult.rawText = fullPageText;
    pageResult.normalizedText = fullPageText;
    pageResult.ocrTimeMs = 0.0;
    pageResult.preprocessTimeMs = 0.0;
    pageResult.renderTimeMs = timer.elapsedMs();

    EDMO_LOG_DEBUG("Native text extracted for page {} in {:.2f} ms (blocks: {}, lines: {})",
        pageNumber, pageResult.renderTimeMs, pageResult.blocks.size(), lines.size());

    return Result<OcrPageResult>::success(std::move(pageResult));
}

} // namespace ocr::ingestion
