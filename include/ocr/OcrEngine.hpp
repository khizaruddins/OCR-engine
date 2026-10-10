#pragma once

#include <ocr/Document.hpp>
#include <ocr/IOcrProvider.hpp>
#include <ocr/IPdfRenderer.hpp>
#include <ocr/IPreprocessor.hpp>
#include <ocr/OcrConfig.hpp>
#include <ocr/OcrResult.hpp>
#include <ocr/Result.hpp>

#include <filesystem>
#include <memory>

namespace ocr {

/**
 * @brief Main pipeline orchestrator and public C++ library API for OCR Engine.
 */
class OcrEngine {
public:
    /**
     * @brief Initialize OCR engine with configuration.
     */
    explicit OcrEngine(OcrConfig config = OcrConfig::getProfile("default"));
    ~OcrEngine();

    OcrEngine(const OcrEngine&) = delete;
    OcrEngine& operator=(const OcrEngine&) = delete;
    OcrEngine(OcrEngine&&) noexcept;
    OcrEngine& operator=(OcrEngine&&) noexcept;

    /**
     * @brief Process an input document file (PDF or Image) end-to-end.
     */
    Result<OcrResult> process(const std::filesystem::path& documentPath);

    /**
     * @brief Run preflight inspection only without full extraction/OCR.
     */
    Result<ingestion::PreflightSummary> preflight(const std::filesystem::path& documentPath);

    /**
     * @brief Process an already loaded Document struct.
     */
    Result<OcrResult> process(const Document& document);

    /**
     * @brief Process an individual single Page.
     */
    Result<OcrPageResult> processPage(const Page& page, int pageNumber = 1);

    /**
     * @brief Access active configuration.
     */
    [[nodiscard]] const OcrConfig& config() const noexcept { return config_; }

    /**
     * @brief Update active configuration.
     */
    void setConfig(OcrConfig config);

    /**
     * @brief Inject custom OCR Provider (e.g. for testing or new ML backends).
     */
    void setProvider(std::unique_ptr<IOcrProvider> provider);

    /**
     * @brief Inject custom PDF Renderer.
     */
    void setPdfRenderer(std::unique_ptr<IPdfRenderer> renderer);

    /**
     * @brief Inject custom Preprocessor.
     */
    void setPreprocessor(std::unique_ptr<IPreprocessor> preprocessor);

private:
    OcrConfig config_;
    std::unique_ptr<IOcrProvider> provider_;
    std::unique_ptr<IPdfRenderer> pdfRenderer_;
    std::unique_ptr<IPreprocessor> preprocessor_;

    void initializeComponents();
};

} // namespace ocr
