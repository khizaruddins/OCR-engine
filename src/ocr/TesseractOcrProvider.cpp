#include "TesseractOcrProvider.hpp"
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"
#include "../utils/ImageUtils.hpp"
#include "../utils/Timer.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <tesseract/baseapi.h>
#include <tesseract/ocrclass.h>

namespace edmo::ocr {

TesseractOcrProvider::TesseractOcrProvider()
    : api_(std::make_unique<tesseract::TessBaseAPI>()) {}

TesseractOcrProvider::~TesseractOcrProvider() {
    if (api_) {
        api_->End();
    }
}

std::string TesseractOcrProvider::version() const {
    return TESSERACT_VERSION_STR;
}

std::string TesseractOcrProvider::resolveTessdataDirectory(const std::string& customDir) {
    if (!customDir.empty() && std::filesystem::exists(customDir)) {
        return std::filesystem::absolute(customDir).string();
    }

    // Check project models directory first for fast integer LSTM models
    static const std::vector<std::string> projectPaths = {
        "models/tessdata",
        "models/tessdata_best",
        "../models/tessdata",
        "../models/tessdata_best"
    };
    for (const auto& p : projectPaths) {
        if (std::filesystem::exists(p) && std::filesystem::exists(std::filesystem::path(p) / "eng.traineddata")) {
            std::string absPath = std::filesystem::absolute(p).string();
            setenv("TESSDATA_PREFIX", absPath.c_str(), 1);
            return absPath;
        }
    }

    const char* env = std::getenv("TESSDATA_PREFIX");
    if (env && std::filesystem::exists(env)) {
        if (std::filesystem::is_directory(env)) {
            if (std::filesystem::exists(std::filesystem::path(env) / "eng.traineddata")) {
                return env;
            }
        }
        auto p = std::filesystem::path(env);
        if (p.has_parent_path() && std::filesystem::exists(p.parent_path() / "eng.traineddata")) {
            return p.parent_path().string();
        }
    }

    static const std::vector<std::string> searchPaths = {
        "/usr/share/tesseract-ocr/4.00/tessdata",
        "/usr/share/tesseract-ocr/5/tessdata",
        "/usr/share/tessdata",
        "/usr/local/share/tessdata",
        "/opt/homebrew/share/tessdata"
    };

    for (const auto& p : searchPaths) {
        if (std::filesystem::exists(p) && std::filesystem::exists(std::filesystem::path(p) / "eng.traineddata")) {
            setenv("TESSDATA_PREFIX", p.c_str(), 1);
            return p;
        }
    }

    return "";
}

static std::string normalizeString(const std::string& input) {
    std::istringstream stream(input);
    std::string line;
    std::string normalized;

    while (std::getline(stream, line)) {
        // Remove trailing carriage returns and whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        if (!line.empty() || !normalized.empty()) {
            normalized += line + "\n";
        }
    }

    while (!normalized.empty() && normalized.back() == '\n') {
        normalized.pop_back();
    }

    return normalized;
}

Result<OcrPageResult> TesseractOcrProvider::process(
    const Image& image,
    const OcrOptions& options,
    int pageNumber
) {
    if (image.empty()) {
        return Result<OcrPageResult>::failure(
            ErrorCode::OcrProcessingFailed,
            "Input image is empty",
            "",
            pageNumber
        );
    }

    Timer timer;
    std::string tessdataDir = resolveTessdataDirectory(options.tessdataDir);
    const char* dataPathPtr = tessdataDir.empty() ? nullptr : tessdataDir.c_str();

    if (!api_) {
        api_ = std::make_unique<tesseract::TessBaseAPI>();
    }

    // Only re-initialize API if configuration or language changed
    if (currentLang_ != options.language || currentOem_ != options.oem || currentTessdataDir_ != tessdataDir) {
        auto oem = static_cast<tesseract::OcrEngineMode>(options.oem);
        EDMO_LOG_DEBUG("Initializing Tesseract (lang: {}, psm: {}, oem: {}, datapath: {})",
            options.language, options.psm, options.oem, (dataPathPtr ? dataPathPtr : "default"));

        int initRes = api_->Init(dataPathPtr, options.language.c_str(), oem);
        if (initRes != 0) {
            return Result<OcrPageResult>::failure(
                ErrorCode::OcrInitializationFailed,
                "Failed to initialize Tesseract for language '" + options.language +
                    "' (datapath: " + (dataPathPtr ? dataPathPtr : "default") + ")",
                "",
                pageNumber
            );
        }

        currentLang_ = options.language;
        currentOem_ = options.oem;
        currentTessdataDir_ = tessdataDir;

        api_->SetVariable("debug_file", "/dev/null");
        api_->SetVariable("preserve_interword_spaces", "1");
    }

    api_->SetPageSegMode(static_cast<tesseract::PageSegMode>(options.psm));

    if (!options.charWhitelist.empty()) {
        api_->SetVariable("tessedit_char_whitelist", options.charWhitelist.c_str());
    }
    if (!options.charBlacklist.empty()) {
        api_->SetVariable("tessedit_char_blacklist", options.charBlacklist.c_str());
    }

    Pix* pix = ImageUtils::matToPix(image.mat());
    if (!pix) {
        return Result<OcrPageResult>::failure(
            ErrorCode::OcrProcessingFailed,
            "Failed to convert image to Leptonica Pix format",
            "",
            pageNumber
        );
    }

    api_->SetImage(pix);
    api_->SetSourceResolution(300);

    int recogRes = api_->Recognize(nullptr);
    if (recogRes != 0) {
        pixDestroy(&pix);
        api_->Clear();
        return Result<OcrPageResult>::failure(
            ErrorCode::OcrProcessingFailed,
            "Tesseract recognition failed",
            "",
            pageNumber
        );
    }

    char* utf8Text = api_->GetUTF8Text();
    std::string rawText = utf8Text ? utf8Text : "";
    delete[] utf8Text;

    OcrPageResult pageResult;
    pageResult.pageNumber = pageNumber;
    pageResult.width = image.width();
    pageResult.height = image.height();
    pageResult.rawText = rawText;
    pageResult.normalizedText = normalizeString(rawText);

    // Build hierarchy: Block -> Line -> Token
    tesseract::ResultIterator* ri = api_->GetIterator();
    if (ri != nullptr) {
        OcrBlock currentBlock;
        OcrLine currentLine;
        bool inBlock = false;
        bool inLine = false;

        double totalTokenConf = 0.0;
        size_t tokenCount = 0;

        do {
            if (ri->IsAtBeginningOf(tesseract::RIL_BLOCK)) {
                if (inLine) {
                    currentBlock.lines.push_back(std::move(currentLine));
                    currentLine = OcrLine();
                    inLine = false;
                }
                if (inBlock && (!currentBlock.lines.empty() || !currentBlock.text.empty())) {
                    pageResult.blocks.push_back(std::move(currentBlock));
                }

                currentBlock = OcrBlock();
                inBlock = true;

                int bx1 = 0, by1 = 0, bx2 = 0, by2 = 0;
                ri->BoundingBox(tesseract::RIL_BLOCK, &bx1, &by1, &bx2, &by2);
                currentBlock.bbox = BoundingBox(bx1, by1, std::max(0, bx2 - bx1), std::max(0, by2 - by1));
                currentBlock.confidence = std::clamp(ri->Confidence(tesseract::RIL_BLOCK) / 100.0f, 0.0f, 1.0f);
            }

            if (ri->IsAtBeginningOf(tesseract::RIL_TEXTLINE)) {
                if (inLine) {
                    currentBlock.lines.push_back(std::move(currentLine));
                }

                currentLine = OcrLine();
                inLine = true;

                int lx1 = 0, ly1 = 0, lx2 = 0, ly2 = 0;
                ri->BoundingBox(tesseract::RIL_TEXTLINE, &lx1, &ly1, &lx2, &ly2);
                currentLine.bbox = BoundingBox(lx1, ly1, std::max(0, lx2 - lx1), std::max(0, ly2 - ly1));
                currentLine.confidence = std::clamp(ri->Confidence(tesseract::RIL_TEXTLINE) / 100.0f, 0.0f, 1.0f);

                char* lineText = ri->GetUTF8Text(tesseract::RIL_TEXTLINE);
                if (lineText) {
                    currentLine.text = lineText;
                    delete[] lineText;
                    // Clean line text
                    while (!currentLine.text.empty() && (currentLine.text.back() == '\n' || currentLine.text.back() == '\r')) {
                        currentLine.text.pop_back();
                    }
                }
            }

            // Word token
            char* wordText = ri->GetUTF8Text(tesseract::RIL_WORD);
            if (wordText) {
                std::string wordStr = wordText;
                delete[] wordText;

                float wordConf = std::clamp(ri->Confidence(tesseract::RIL_WORD) / 100.0f, 0.0f, 1.0f);

                int wx1 = 0, wy1 = 0, wx2 = 0, wy2 = 0;
                ri->BoundingBox(tesseract::RIL_WORD, &wx1, &wy1, &wx2, &wy2);
                BoundingBox wordBox(wx1, wy1, std::max(0, wx2 - wx1), std::max(0, wy2 - wy1));

                if (wordConf >= options.confidenceThreshold && !wordStr.empty()) {
                    OcrToken token;
                    token.text = wordStr;
                    token.confidence = wordConf;
                    token.bbox = wordBox;

                    totalTokenConf += static_cast<double>(wordConf);
                    tokenCount++;

                    currentLine.tokens.push_back(std::move(token));
                }
            }

        } while (ri->Next(tesseract::RIL_WORD));

        if (inLine) {
            currentBlock.lines.push_back(std::move(currentLine));
        }
        if (inBlock && (!currentBlock.lines.empty() || !currentBlock.text.empty())) {
            pageResult.blocks.push_back(std::move(currentBlock));
        }

        delete ri;

        if (tokenCount > 0) {
            pageResult.confidence = static_cast<float>(totalTokenConf / tokenCount);
        } else if (!pageResult.blocks.empty()) {
            float totalBlockConf = 0.0f;
            for (const auto& b : pageResult.blocks) totalBlockConf += b.confidence;
            pageResult.confidence = totalBlockConf / pageResult.blocks.size();
        } else {
            pageResult.confidence = 0.0f;
        }
    }

    pixDestroy(&pix);
    api_->Clear();

    pageResult.ocrTimeMs = timer.elapsedMs();
    EDMO_LOG_DEBUG("OCR page {} completed in {:.2f} ms (confidence: {:.2f}%, blocks: {})",
        pageNumber, pageResult.ocrTimeMs, pageResult.confidence * 100.0f, pageResult.blocks.size());

    return Result<OcrPageResult>::success(std::move(pageResult));
}

Result<std::string> TesseractOcrProvider::processCell(
    const Image& cellImage,
    int psm,
    const std::string& charWhitelist,
    float* outConfidence
) {
    if (cellImage.empty()) {
        return Result<std::string>::failure(
            ErrorCode::OcrProcessingFailed,
            "Cell image is empty"
        );
    }

    if (!api_) {
        api_ = std::make_unique<tesseract::TessBaseAPI>();
        std::string tessdataDir = resolveTessdataDirectory("");
        const char* dataPathPtr = tessdataDir.empty() ? nullptr : tessdataDir.c_str();
        int initRes = api_->Init(dataPathPtr, "eng", tesseract::OEM_LSTM_ONLY);
        if (initRes != 0) {
            return Result<std::string>::failure(
                ErrorCode::OcrInitializationFailed,
                "Failed to initialize Tesseract for cell OCR"
            );
        }
    }

    api_->SetPageSegMode(static_cast<tesseract::PageSegMode>(psm));
    if (!charWhitelist.empty()) {
        api_->SetVariable("tessedit_char_whitelist", charWhitelist.c_str());
    } else {
        api_->SetVariable("tessedit_char_whitelist", "");
    }

    Pix* pix = ImageUtils::matToPix(cellImage.mat());
    if (!pix) {
        return Result<std::string>::failure(
            ErrorCode::OcrProcessingFailed,
            "Failed to convert cell image to Leptonica Pix"
        );
    }

    api_->SetImage(pix);
    api_->SetSourceResolution(300);

    int recogRes = api_->Recognize(nullptr);
    if (recogRes != 0) {
        pixDestroy(&pix);
        api_->Clear();
        return Result<std::string>::failure(
            ErrorCode::OcrProcessingFailed,
            "Cell recognition failed"
        );
    }

    char* utf8Text = api_->GetUTF8Text();
    std::string text = utf8Text ? utf8Text : "";
    delete[] utf8Text;

    if (outConfidence != nullptr) {
        *outConfidence = std::clamp(api_->MeanTextConf() / 100.0f, 0.0f, 1.0f);
    }

    pixDestroy(&pix);
    api_->Clear();

    // Reset whitelist to avoid polluting subsequent calls
    if (!charWhitelist.empty()) {
        api_->SetVariable("tessedit_char_whitelist", "");
    }

    // Clean whitespace
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.pop_back();
    }
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        text = text.substr(start);
    } else {
        text.clear();
    }

    return Result<std::string>::success(std::move(text));
}

} // namespace edmo::ocr
