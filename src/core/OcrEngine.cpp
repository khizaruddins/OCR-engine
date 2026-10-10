#include <ocr/OcrEngine.hpp>
#include <ocr/ErrorCode.hpp>
#include <ocr/ingestion/DocumentIngestor.hpp>
#include <ocr/ingestion/NativeTextExtractor.hpp>
#include <ocr/ingestion/PageComplexityAnalyzer.hpp>
#include "../logging/Logger.hpp"
#include "../ocr/TesseractOcrProvider.hpp"
#include "../pdf/PdfRenderer.hpp"
#include "../preprocessing/ImagePreprocessor.hpp"
#include "../utils/CoordinateTransform.hpp"
#include "../utils/FileUtils.hpp"
#include "../utils/ImageUtils.hpp"
#include "../utils/Timer.hpp"

#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

namespace ocr {

OcrEngine::OcrEngine(OcrConfig config)
    : config_(std::move(config)) {
    Logger::init(config_.debug.enabled ? LogLevel::Debug : LogLevel::Info);
    initializeComponents();
}

OcrEngine::~OcrEngine() = default;
OcrEngine::OcrEngine(OcrEngine&&) noexcept = default;
OcrEngine& OcrEngine::operator=(OcrEngine&&) noexcept = default;

void OcrEngine::initializeComponents() {
    if (!provider_) {
        provider_ = createOcrProvider(config_);
    }
    if (!pdfRenderer_) {
        pdfRenderer_ = std::make_unique<PdfRenderer>();
    }
    if (!preprocessor_) {
        preprocessor_ = std::make_unique<ImagePreprocessor>();
    }
}

void OcrEngine::setConfig(OcrConfig config) {
    config_ = std::move(config);
    if (config_.debug.enabled) {
        Logger::setLevel(LogLevel::Debug);
    }
    // Reinitialize provider if provider settings changed
    provider_ = createOcrProvider(config_);
}

void OcrEngine::setProvider(std::unique_ptr<IOcrProvider> provider) {
    provider_ = std::move(provider);
}

void OcrEngine::setPdfRenderer(std::unique_ptr<IPdfRenderer> renderer) {
    pdfRenderer_ = std::move(renderer);
}

void OcrEngine::setPreprocessor(std::unique_ptr<IPreprocessor> preprocessor) {
    preprocessor_ = std::move(preprocessor);
}

Result<ingestion::PreflightSummary> OcrEngine::preflight(const std::filesystem::path& documentPath) {
    ingestion::DocumentIngestor ingestor(config_.ingestion);
    return ingestor.ingest(documentPath);
}

Result<OcrPageResult> OcrEngine::processPage(const Page& page, int pageNumber) {
    if (page.image.empty()) {
        return Result<OcrPageResult>::failure(
            ErrorCode::InvalidDocument,
            "Page image is empty",
            "",
            pageNumber
        );
    }

    if (!provider_) {
        return Result<OcrPageResult>::failure(
            ErrorCode::OcrInitializationFailed,
            "OCR provider is not initialized",
            "",
            pageNumber
        );
    }

    DebugContext debugCtx;
    debugCtx.enabled = config_.debug.enabled && config_.debug.saveIntermediateImages;
    debugCtx.outputDir = config_.debug.outputDir;
    debugCtx.pageNumber = pageNumber;

    // 1. Preprocessing stage
    auto prepRes = preprocessor_->process(page.image, config_.preprocessing, debugCtx);
    if (!prepRes) {
        return Result<OcrPageResult>::failure(prepRes.error());
    }

    const auto& preprocessed = prepRes.value();

    // 2. OCR recognition stage
    auto ocrRes = provider_->process(preprocessed.image, config_.toOcrOptions(), pageNumber);
    if (!ocrRes) {
        return Result<OcrPageResult>::failure(ocrRes.error());
    }

    auto pageResult = std::move(ocrRes.value());
    int appliedRotation = preprocessed.detectedRotationDegrees;

    // 3. Fallback Quality Check: if OCR confidence is critically low (< 40%) and rotation was 0°, evaluate candidate rotation
    if (pageResult.confidence < 0.40f && appliedRotation == 0 && config_.preprocessing.orientation) {
        static const std::vector<int> candidateAngles = {90, 270, 180};
        for (int candAngle : candidateAngles) {
            cv::Mat candRotated = OrientationDetector::applyRotation(preprocessed.image.mat(), candAngle);
            auto candOcrRes = provider_->process(Image(candRotated), config_.toOcrOptions(), pageNumber);
            if (candOcrRes && candOcrRes.value().confidence > (pageResult.confidence + 0.25f)) {
                EDMO_LOG_INFO("Quality gate triggered orientation fallback: rotated {}° improved confidence from {:.2f}% to {:.2f}%",
                    candAngle, pageResult.confidence * 100.0f, candOcrRes.value().confidence * 100.0f);
                pageResult = std::move(candOcrRes.value());
                appliedRotation = candAngle;
                break;
            }
        }
    }

    // 4. Invert OCR coordinates back to original unrotated image coordinates
    if (appliedRotation != 0) {
        CoordinateTransform::invertPageCoordinates(
            pageResult,
            appliedRotation,
            preprocessed.originalWidth,
            preprocessed.originalHeight
        );
    } else {
        pageResult.width = preprocessed.originalWidth;
        pageResult.height = preprocessed.originalHeight;
    }

    pageResult.preprocessTimeMs = preprocessed.durationMs;
    pageResult.rotation = appliedRotation;
    pageResult.skewAngle = preprocessed.detectedSkewDegrees;
    pageResult.sourceType = "OCR";
    pageResult.processingStrategy = "OCR";
    pageResult.pageClassification = "SCANNED_IMAGE";

    return Result<OcrPageResult>::success(std::move(pageResult));
}

Result<OcrResult> OcrEngine::process(const std::filesystem::path& documentPath) {
    Timer totalTimer;
    EDMO_LOG_INFO("Processing document: {}", documentPath.string());

    // Synchronize limits from config_
    config_.ingestion.maxPages = std::min({config_.ingestion.maxPages, config_.pdf.maxPages, config_.limits.maxPages});
    config_.ingestion.maxFileSizeMb = std::min(config_.ingestion.maxFileSizeMb, config_.limits.maxFileSizeMb);

    // 1. Document Ingestion & Preflight
    ingestion::DocumentIngestor ingestor(config_.ingestion);
    auto preflightRes = ingestor.ingest(documentPath);
    if (!preflightRes) {
        EDMO_LOG_ERROR("Preflight validation failed: {}", preflightRes.error().message);
        return Result<OcrResult>::failure(preflightRes.error());
    }

    auto preflightSummary = std::move(preflightRes.value());

    OcrResult result;
    result.id = FileUtils::generateProcessingId();
    result.schemaVersion = "0.1";
    result.engine.name = "ocr-engine";
    result.engine.version = "0.2.0";
    result.engine.provider = provider_ ? provider_->name() : "tesseract";

    result.document.id = FileUtils::generateDocumentId();
    result.document.source = documentPath.string();
    result.document.type = preflightSummary.format == "PDF" ? "pdf" : "image";
    result.document.pageCount = preflightSummary.pageCount;
    result.document.fileSizeBytes = preflightSummary.fileSizeBytes;

    result.preflight = preflightSummary;
    result.documentClassification = std::string(ingestion::documentLevelTypeToString(preflightSummary.documentType));
    result.strategy = config_.ingestion.forceOcr ? "FORCE_OCR" : (config_.ingestion.forceNativeText ? "FORCE_NATIVE" : "ADAPTIVE");

    ingestion::NativeTextExtractor nativeExtractor(config_.ingestion);
    ingestion::PageComplexityAnalyzer complexityAnalyzer;

    double totalPreprocessTime = 0.0;
    double totalOcrTime = 0.0;
    double totalNativeExtractionTime = 0.0;
    double totalPdfRenderTime = 0.0;
    float sumConfidence = 0.0f;

    if (preflightSummary.format == "PDF") {
        std::unique_ptr<poppler::document> popplerDoc(poppler::document::load_from_file(documentPath.string()));
        poppler::page_renderer popplerRenderer;
        popplerRenderer.set_render_hint(poppler::page_renderer::antialiasing, true);
        popplerRenderer.set_render_hint(poppler::page_renderer::text_antialiasing, true);

        for (auto& pInspect : preflightSummary.pages) {
            Timer pageTotalTimer;
            int pageNum = pInspect.pageNumber;

            if (pInspect.strategy == ingestion::ProcessingStrategyType::NativeText) {
                // Strategy: NATIVE_TEXT
                Timer nativeTimer;
                auto nativeRes = nativeExtractor.extractPage(documentPath, pageNum, config_.pdf.dpi);

                if (nativeRes) {
                    auto pResult = std::move(nativeRes.value());
                    pResult.sourceType = "NATIVE_TEXT";
                    pResult.processingStrategy = "NATIVE_TEXT";
                    pResult.pageClassification = std::string(ingestion::pageInputTypeToString(pInspect.inputType));
                    pResult.timing.preflightMs = pInspect.timing.preflightMs;
                    pResult.timing.nativeExtractionMs = nativeTimer.elapsedMs();
                    pResult.timing.ocrMs = 0.0;
                    pResult.timing.totalMs = pageTotalTimer.elapsedMs();
                    pResult.complexity = complexityAnalyzer.analyze(pResult);

                    totalNativeExtractionTime += pResult.timing.nativeExtractionMs;
                    sumConfidence += pResult.confidence;

                    result.pages.push_back(std::move(pResult));
                    continue;
                } else {
                    // Fallback to OCR
                    EDMO_LOG_WARN("Native text extraction failed for page {}, triggering OCR fallback: {}",
                        pageNum, nativeRes.error().message);
                    pInspect.fallbackTriggered = true;
                    pInspect.fallbackFrom = "NATIVE_TEXT";
                    pInspect.fallbackTo = "OCR";
                    pInspect.fallbackReason = nativeRes.error().message;
                    pInspect.strategy = ingestion::ProcessingStrategyType::Ocr;
                }
            }

            if (pInspect.strategy == ingestion::ProcessingStrategyType::SkipEmpty) {
                OcrPageResult emptyPage;
                emptyPage.pageNumber = pageNum;
                emptyPage.width = pInspect.width;
                emptyPage.height = pInspect.height;
                emptyPage.confidence = 1.0f;
                emptyPage.sourceType = "SKIP_EMPTY";
                emptyPage.processingStrategy = "SKIP_EMPTY";
                emptyPage.pageClassification = "EMPTY";
                emptyPage.timing.preflightMs = pInspect.timing.preflightMs;
                emptyPage.timing.totalMs = pageTotalTimer.elapsedMs();
                emptyPage.complexity.level = ingestion::PageComplexityLevel::Simple;
                emptyPage.complexity.score = 0.0;

                sumConfidence += 1.0f;
                result.pages.push_back(std::move(emptyPage));
                continue;
            }

            // Strategy: OCR
            Timer renderTimer;
            std::unique_ptr<poppler::page> p(popplerDoc ? popplerDoc->create_page(pageNum - 1) : nullptr);
            if (!p) {
                return Result<OcrResult>::failure(
                    ErrorCode::PdfRenderingFailed,
                    "Failed to access PDF page for OCR rendering: " + std::to_string(pageNum),
                    documentPath.string(),
                    pageNum
                );
            }

            poppler::image renderedImg = popplerRenderer.render_page(p.get(), config_.pdf.dpi, config_.pdf.dpi);
            if (!renderedImg.is_valid()) {
                return Result<OcrResult>::failure(
                    ErrorCode::PdfRenderingFailed,
                    "Failed to render page image for OCR on page " + std::to_string(pageNum),
                    documentPath.string(),
                    pageNum
                );
            }

            auto matRes = ImageUtils::popplerImageToMat(renderedImg);
            if (!matRes) {
                return Result<OcrResult>::failure(matRes.error());
            }

            double pageRenderTime = renderTimer.elapsedMs();
            totalPdfRenderTime += pageRenderTime;

            Page page;
            page.pageNumber = pageNum;
            page.width = matRes.value().cols;
            page.height = matRes.value().rows;
            page.dpi = config_.pdf.dpi;
            page.image = Image(matRes.value());

            auto pageRes = processPage(page, pageNum);
            if (!pageRes) {
                EDMO_LOG_ERROR("OCR failed on page {}: {}", pageNum, pageRes.error().message);
                return Result<OcrResult>::failure(pageRes.error());
            }

            auto pResult = std::move(pageRes.value());
            pResult.sourceType = "OCR";
            pResult.processingStrategy = std::string(ingestion::processingStrategyToString(pInspect.strategy));
            pResult.pageClassification = std::string(ingestion::pageInputTypeToString(pInspect.inputType));
            pResult.renderTimeMs = pageRenderTime;
            pResult.timing.preflightMs = pInspect.timing.preflightMs;
            pResult.timing.ocrMs = pResult.ocrTimeMs;
            pResult.timing.nativeExtractionMs = 0.0;

            // If native text exists on the page, extract and preserve native evidence
            if (pInspect.textLayer.exists) {
                Timer nativeTimer;
                auto nativeRes = nativeExtractor.extractPage(documentPath, pageNum, config_.pdf.dpi);
                if (nativeRes) {
                    pResult.nativeBlocks = std::move(nativeRes.value().blocks);
                    pResult.hasConflict = !pInspect.textLayer.reliability.isReliable;
                    pResult.conflictReason = pInspect.textLayer.reliability.triggerReason;
                    pResult.timing.nativeExtractionMs = nativeTimer.elapsedMs();
                    totalNativeExtractionTime += pResult.timing.nativeExtractionMs;
                }
            }

            pResult.timing.totalMs = pageTotalTimer.elapsedMs();
            pResult.complexity = complexityAnalyzer.analyze(pResult);

            totalPreprocessTime += pResult.preprocessTimeMs;
            totalOcrTime += pResult.ocrTimeMs;
            sumConfidence += pResult.confidence;

            result.pages.push_back(std::move(pResult));
        }
    } else {
        // Image Processing Path
        auto imgRes = ImageUtils::loadMat(documentPath, config_.limits.maxImageDimension);
        if (!imgRes) {
            return Result<OcrResult>::failure(imgRes.error());
        }

        Page page;
        page.pageNumber = 1;
        page.width = imgRes.value().cols;
        page.height = imgRes.value().rows;
        page.dpi = config_.pdf.dpi;
        page.image = Image(imgRes.value());

        auto pageRes = processPage(page, 1);
        if (!pageRes) {
            return Result<OcrResult>::failure(pageRes.error());
        }

        auto pResult = std::move(pageRes.value());
        pResult.sourceType = "OCR";
        pResult.processingStrategy = "OCR";
        pResult.pageClassification = "SCANNED_IMAGE";
        pResult.complexity = complexityAnalyzer.analyze(pResult);

        totalPreprocessTime += pResult.preprocessTimeMs;
        totalOcrTime += pResult.ocrTimeMs;
        sumConfidence += pResult.confidence;

        result.pages.push_back(std::move(pResult));
    }

    result.confidence = result.pages.empty() ? 0.0f : (sumConfidence / result.pages.size());
    result.timing.preflightTimeMs = preflightSummary.performance.preflightTimeMs;
    result.timing.nativeExtractionTimeMs = totalNativeExtractionTime;
    result.timing.pdfRenderTimeMs = totalPdfRenderTime;
    result.timing.preprocessTimeMs = totalPreprocessTime;
    result.timing.ocrTimeMs = totalOcrTime;
    result.timing.totalTimeMs = totalTimer.elapsedMs();

    EDMO_LOG_INFO("Document processing completed successfully in {:.2f} ms (pages: {}, avg confidence: {:.2f}%, type: {})",
        result.timing.totalTimeMs, result.pages.size(), result.confidence * 100.0f, result.documentClassification);

    return Result<OcrResult>::success(std::move(result));
}

Result<OcrResult> OcrEngine::process(const Document& document) {
    if (document.empty()) {
        return Result<OcrResult>::failure(
            ErrorCode::InvalidDocument,
            "Document contains no pages",
            document.sourcePath
        );
    }

    Timer timer;
    OcrResult result;
    result.id = FileUtils::generateProcessingId();
    result.schemaVersion = "0.2";
    result.engine.name = "ocr-engine";
    result.engine.version = "0.2.0";
    result.engine.provider = provider_ ? provider_->name() : "unknown";

    result.document.id = document.id;
    result.document.source = document.sourcePath;
    result.document.type = std::string(documentTypeToString(document.type));
    result.document.pageCount = static_cast<int>(document.pages.size());
    result.document.fileSizeBytes = document.fileSizeBytes;

    ingestion::PageComplexityAnalyzer complexityAnalyzer;
    double totalPreprocessTime = 0.0;
    double totalOcrTime = 0.0;
    float sumConfidence = 0.0f;

    for (const auto& page : document.pages) {
        auto pageRes = processPage(page, page.pageNumber);
        if (!pageRes) {
            EDMO_LOG_ERROR("Failed to process page {}: {}", page.pageNumber, pageRes.error().message);
            return Result<OcrResult>::failure(pageRes.error());
        }

        auto pResult = std::move(pageRes.value());
        pResult.sourceType = "OCR";
        pResult.processingStrategy = "OCR";
        pResult.pageClassification = "SCANNED_IMAGE";
        pResult.complexity = complexityAnalyzer.analyze(pResult);

        totalPreprocessTime += pResult.preprocessTimeMs;
        totalOcrTime += pResult.ocrTimeMs;
        sumConfidence += pResult.confidence;

        result.pages.push_back(std::move(pResult));
    }

    result.confidence = result.pages.empty() ? 0.0f : (sumConfidence / result.pages.size());
    result.timing.preprocessTimeMs = totalPreprocessTime;
    result.timing.ocrTimeMs = totalOcrTime;
    result.timing.totalTimeMs = timer.elapsedMs();

    EDMO_LOG_INFO("Document processing completed successfully in {:.2f} ms (pages: {}, avg confidence: {:.2f}%)",
        result.timing.totalTimeMs, result.pages.size(), result.confidence * 100.0f);

    return Result<OcrResult>::success(std::move(result));
}

} // namespace ocr
