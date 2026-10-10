#pragma once

#include <edmo/ocr/OcrOptions.hpp>
#include <edmo/ocr/Result.hpp>
#include <edmo/ocr/Types.hpp>
#include <edmo/ocr/ingestion/IngestionConfig.hpp>

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace edmo::ocr {

struct OcrBackendConfig {
    std::string provider = "tesseract"; // tesseract, onnx, paddleocr, edmo
    std::string language = "eng";
    int psm = 3; // PSM_AUTO
    int oem = 1; // OEM_LSTM_ONLY
    std::string tessdataDir;
    std::string charWhitelist;
    std::string charBlacklist;
    std::string device = "cpu"; // cpu, cuda
    float confidenceThreshold = 0.0f;
};

struct PdfConfig {
    int dpi = 300; // 150, 200, 300, 400, 600
    int maxPages = 100;
};

struct PreprocessingConfig {
    bool enabled = true;
    bool grayscale = true;
    bool denoise = true;
    DenoiseMethod denoiseMethod = DenoiseMethod::Gaussian;
    int denoiseKernelSize = 3;
    double denoiseSigma = 1.0;

    bool contrast = true;
    ContrastMethod contrastMethod = ContrastMethod::Clahe;
    double claheClipLimit = 2.0;
    int claheGridSize = 8;

    bool binarization = false;
    BinarizationMethod binarizationMethod = BinarizationMethod::Otsu;
    double globalThreshold = 128.0;
    int adaptiveBlockSize = 11;
    double adaptiveC = 2.0;

    bool deskew = true;
    DeskewMethod deskewMethod = DeskewMethod::HoughLines;
    double maxSkewAngleDegrees = 10.0;
    double minSkewAngleThreshold = 0.2;

    bool orientation = true; // Auto orientation detection (0, 90, 180, 270)
    bool sharpen = false;
    double sharpenStrength = 1.0;
};

struct OutputConfig {
    OutputFormat format = OutputFormat::FullJson;
    bool includeTokens = true;
    bool includeLines = true;
    bool includeBlocks = true;
    bool includeRawText = true;
    bool includeNormalizedText = true;
    bool includeGeometry = true;
    bool includeTimings = true;
    bool prettyPrint = true;
};

struct LimitsConfig {
    size_t maxFileSizeMb = 50;
    int maxPages = 100;
    int maxImageDimension = 20000;
};

struct DebugConfig {
    bool enabled = false;
    std::string outputDir = "debug";
    bool saveIntermediateImages = true;
};

/**
 * @brief Top-level configuration object for the EDMO OCR Engine.
 */
struct OcrConfig {
    OcrBackendConfig ocr;
    PdfConfig pdf;
    PreprocessingConfig preprocessing;
    OutputConfig output;
    LimitsConfig limits;
    DebugConfig debug;
    ingestion::IngestionConfig ingestion;

    /**
     * @brief Generate pre-configured profile configuration.
     * Supported profiles: default, clean-document, scanned-document, low-quality-scan, transcript.
     */
    static OcrConfig getProfile(const std::string& profileName);

    /**
     * @brief Parse configuration from JSON object.
     */
    static Result<OcrConfig> fromJson(const nlohmann::json& json);

    /**
     * @brief Parse configuration from JSON file.
     */
    static Result<OcrConfig> fromFile(const std::filesystem::path& path);

    /**
     * @brief Serialize current configuration to JSON.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief Convert configuration to request-level OcrOptions.
     */
    [[nodiscard]] OcrOptions toOcrOptions() const;
};

} // namespace edmo::ocr
