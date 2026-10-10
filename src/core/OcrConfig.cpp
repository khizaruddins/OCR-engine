#include <ocr/OcrConfig.hpp>
#include <ocr/ErrorCode.hpp>

#include <fstream>
#include <sstream>

namespace ocr {

OcrConfig OcrConfig::getProfile(const std::string& profileName) {
    OcrConfig config;

    if (profileName == "clean-document") {
        config.preprocessing.enabled = true;
        config.preprocessing.grayscale = true;
        config.preprocessing.denoise = false;
        config.preprocessing.contrast = false;
        config.preprocessing.binarization = false;
        config.preprocessing.deskew = false;
        config.preprocessing.orientation = false;
        config.preprocessing.sharpen = false;
    } else if (profileName == "scanned-document") {
        config.preprocessing.enabled = true;
        config.preprocessing.grayscale = true;
        config.preprocessing.denoise = true;
        config.preprocessing.denoiseMethod = DenoiseMethod::Gaussian;
        config.preprocessing.contrast = true;
        config.preprocessing.contrastMethod = ContrastMethod::Clahe;
        config.preprocessing.binarization = false;
        config.preprocessing.deskew = true;
        config.preprocessing.orientation = true;
    } else if (profileName == "low-quality-scan") {
        config.preprocessing.enabled = true;
        config.preprocessing.grayscale = true;
        config.preprocessing.denoise = true;
        config.preprocessing.denoiseMethod = DenoiseMethod::Median;
        config.preprocessing.denoiseKernelSize = 3;
        config.preprocessing.contrast = true;
        config.preprocessing.contrastMethod = ContrastMethod::Clahe;
        config.preprocessing.claheClipLimit = 3.0;
        config.preprocessing.binarization = true;
        config.preprocessing.binarizationMethod = BinarizationMethod::Otsu;
        config.preprocessing.deskew = true;
        config.preprocessing.orientation = true;
        config.preprocessing.sharpen = true;
        config.preprocessing.sharpenStrength = 1.2;
    } else if (profileName == "transcript") {
        // Optimized for academic transcripts with dense tabular text and faint lines
        config.preprocessing.enabled = true;
        config.preprocessing.grayscale = true;
        config.preprocessing.denoise = false; // Digital/Clean transcripts do not need blurring
        config.preprocessing.contrast = true;
        config.preprocessing.contrastMethod = ContrastMethod::Clahe;
        config.preprocessing.claheClipLimit = 1.5;
        config.preprocessing.claheGridSize = 8;
        config.preprocessing.binarization = false; // Preserve anti-aliased font edges
        config.preprocessing.deskew = true;
        config.preprocessing.orientation = true;
        config.preprocessing.sharpen = false;
        config.pdf.dpi = 300;
        config.ocr.psm = 3; // Fully automatic page segmentation
    } else {
        // Default profile: Ultra-fast & accurate for general documents
        config.preprocessing.enabled = true;
        config.preprocessing.grayscale = true;
        config.preprocessing.denoise = false;
        config.preprocessing.contrast = false;
        config.preprocessing.binarization = false;
        config.preprocessing.deskew = true;
        config.preprocessing.orientation = true;
        config.preprocessing.sharpen = false;
        config.pdf.dpi = 300;
    }

    return config;
}

Result<OcrConfig> OcrConfig::fromJson(const nlohmann::json& j) {
    OcrConfig config;

    try {
        if (j.contains("profile") && j["profile"].is_string()) {
            config = getProfile(j["profile"].get<std::string>());
        }

        if (j.contains("ocr")) {
            const auto& o = j["ocr"];
            config.ocr.provider = o.value("provider", config.ocr.provider);
            config.ocr.language = o.value("language", config.ocr.language);
            config.ocr.psm = o.value("psm", config.ocr.psm);
            config.ocr.oem = o.value("oem", config.ocr.oem);
            config.ocr.tessdataDir = o.value("tessdata_dir", config.ocr.tessdataDir);
            config.ocr.charWhitelist = o.value("whitelist", config.ocr.charWhitelist);
            config.ocr.charBlacklist = o.value("blacklist", config.ocr.charBlacklist);
            config.ocr.device = o.value("device", config.ocr.device);
        }

        if (j.contains("pdf")) {
            const auto& p = j["pdf"];
            config.pdf.dpi = p.value("dpi", config.pdf.dpi);
            config.pdf.maxPages = p.value("max_pages", config.pdf.maxPages);
        }

        if (j.contains("preprocessing")) {
            const auto& prep = j["preprocessing"];
            config.preprocessing.enabled = prep.value("enabled", config.preprocessing.enabled);
            config.preprocessing.grayscale = prep.value("grayscale", config.preprocessing.grayscale);
            config.preprocessing.denoise = prep.value("denoise", config.preprocessing.denoise);
            if (prep.contains("denoise_method")) {
                std::string m = prep["denoise_method"];
                if (m == "median") config.preprocessing.denoiseMethod = DenoiseMethod::Median;
                else if (m == "bilateral") config.preprocessing.denoiseMethod = DenoiseMethod::Bilateral;
                else config.preprocessing.denoiseMethod = DenoiseMethod::Gaussian;
            }
            config.preprocessing.denoiseKernelSize = prep.value("denoise_kernel_size", config.preprocessing.denoiseKernelSize);
            config.preprocessing.denoiseSigma = prep.value("denoise_sigma", config.preprocessing.denoiseSigma);

            config.preprocessing.contrast = prep.value("contrast", config.preprocessing.contrast);
            if (prep.contains("contrast_method")) {
                std::string m = prep["contrast_method"];
                if (m == "equalize") config.preprocessing.contrastMethod = ContrastMethod::HistogramEqualization;
                else config.preprocessing.contrastMethod = ContrastMethod::Clahe;
            }
            config.preprocessing.claheClipLimit = prep.value("clahe_clip_limit", config.preprocessing.claheClipLimit);
            config.preprocessing.claheGridSize = prep.value("clahe_grid_size", config.preprocessing.claheGridSize);

            config.preprocessing.binarization = prep.value("binarization", config.preprocessing.binarization);
            if (prep.contains("binarization_method")) {
                std::string m = prep["binarization_method"];
                if (m == "global") config.preprocessing.binarizationMethod = BinarizationMethod::GlobalThreshold;
                else if (m == "adaptive_mean") config.preprocessing.binarizationMethod = BinarizationMethod::AdaptiveMean;
                else if (m == "adaptive_gaussian") config.preprocessing.binarizationMethod = BinarizationMethod::AdaptiveGaussian;
                else config.preprocessing.binarizationMethod = BinarizationMethod::Otsu;
            }
            config.preprocessing.globalThreshold = prep.value("global_threshold", config.preprocessing.globalThreshold);
            config.preprocessing.adaptiveBlockSize = prep.value("adaptive_block_size", config.preprocessing.adaptiveBlockSize);
            config.preprocessing.adaptiveC = prep.value("adaptive_c", config.preprocessing.adaptiveC);

            config.preprocessing.deskew = prep.value("deskew", config.preprocessing.deskew);
            if (prep.contains("deskew_method")) {
                std::string m = prep["deskew_method"];
                if (m == "projection") config.preprocessing.deskewMethod = DeskewMethod::ProjectionProfile;
                else if (m == "min_area_rect") config.preprocessing.deskewMethod = DeskewMethod::MinAreaRect;
                else config.preprocessing.deskewMethod = DeskewMethod::HoughLines;
            }
            config.preprocessing.maxSkewAngleDegrees = prep.value("max_skew_angle_degrees", config.preprocessing.maxSkewAngleDegrees);
            config.preprocessing.minSkewAngleThreshold = prep.value("min_skew_angle_threshold", config.preprocessing.minSkewAngleThreshold);

            config.preprocessing.orientation = prep.value("orientation", config.preprocessing.orientation);
            config.preprocessing.sharpen = prep.value("sharpen", config.preprocessing.sharpen);
            config.preprocessing.sharpenStrength = prep.value("sharpen_strength", config.preprocessing.sharpenStrength);
        }

        if (j.contains("output")) {
            const auto& out = j["output"];
            if (out.contains("format")) {
                std::string fmt = out["format"];
                if (fmt == "compact-json") config.output.format = OutputFormat::CompactJson;
                else if (fmt == "text") config.output.format = OutputFormat::PlainText;
                else config.output.format = OutputFormat::FullJson;
            }
            config.output.includeTokens = out.value("include_tokens", config.output.includeTokens);
            config.output.includeLines = out.value("include_lines", config.output.includeLines);
            config.output.includeBlocks = out.value("include_blocks", config.output.includeBlocks);
            config.output.includeRawText = out.value("include_raw_text", config.output.includeRawText);
            config.output.includeNormalizedText = out.value("include_normalized_text", config.output.includeNormalizedText);
            config.output.includeGeometry = out.value("include_geometry", config.output.includeGeometry);
            config.output.includeTimings = out.value("include_timings", config.output.includeTimings);
            config.output.prettyPrint = out.value("pretty_print", config.output.prettyPrint);
        }

        if (j.contains("limits")) {
            const auto& lim = j["limits"];
            config.limits.maxFileSizeMb = lim.value("max_file_size_mb", config.limits.maxFileSizeMb);
            config.limits.maxPages = lim.value("max_pages", config.limits.maxPages);
            config.limits.maxImageDimension = lim.value("max_image_dimension", config.limits.maxImageDimension);
        }

        if (j.contains("debug")) {
            const auto& dbg = j["debug"];
            config.debug.enabled = dbg.value("enabled", config.debug.enabled);
            config.debug.outputDir = dbg.value("output_dir", config.debug.outputDir);
            config.debug.saveIntermediateImages = dbg.value("save_intermediate_images", config.debug.saveIntermediateImages);
        }

        if (j.contains("ingestion") && j["ingestion"].is_object()) {
            config.ingestion = ingestion::IngestionConfig::fromJson(j["ingestion"]);
        }

        return Result<OcrConfig>::success(config);
    } catch (const std::exception& e) {
        return Result<OcrConfig>::failure(
            ErrorCode::InvalidConfiguration,
            std::string("JSON parsing exception: ") + e.what()
        );
    }
}

Result<OcrConfig> OcrConfig::fromFile(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return Result<OcrConfig>::failure(ErrorCode::FileNotFound, "Configuration file not found", path.string());
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return Result<OcrConfig>::failure(ErrorCode::InvalidConfiguration, "Could not open configuration file", path.string());
    }

    try {
        nlohmann::json j;
        file >> j;
        return fromJson(j);
    } catch (const std::exception& e) {
        return Result<OcrConfig>::failure(
            ErrorCode::InvalidConfiguration,
            std::string("Failed to parse configuration file: ") + e.what(),
            path.string()
        );
    }
}

nlohmann::json OcrConfig::toJson() const {
    std::string denoiseMethodStr = "gaussian";
    if (preprocessing.denoiseMethod == DenoiseMethod::Median) denoiseMethodStr = "median";
    else if (preprocessing.denoiseMethod == DenoiseMethod::Bilateral) denoiseMethodStr = "bilateral";

    std::string contrastMethodStr = (preprocessing.contrastMethod == ContrastMethod::HistogramEqualization) ? "equalize" : "clahe";

    std::string binarizationMethodStr = "otsu";
    if (preprocessing.binarizationMethod == BinarizationMethod::GlobalThreshold) binarizationMethodStr = "global";
    else if (preprocessing.binarizationMethod == BinarizationMethod::AdaptiveMean) binarizationMethodStr = "adaptive_mean";
    else if (preprocessing.binarizationMethod == BinarizationMethod::AdaptiveGaussian) binarizationMethodStr = "adaptive_gaussian";

    std::string deskewMethodStr = "hough";
    if (preprocessing.deskewMethod == DeskewMethod::ProjectionProfile) deskewMethodStr = "projection";
    else if (preprocessing.deskewMethod == DeskewMethod::MinAreaRect) deskewMethodStr = "min_area_rect";

    return nlohmann::json{
        {"ocr", {
            {"provider", ocr.provider},
            {"language", ocr.language},
            {"psm", ocr.psm},
            {"oem", ocr.oem},
            {"tessdata_dir", ocr.tessdataDir},
            {"whitelist", ocr.charWhitelist},
            {"blacklist", ocr.charBlacklist},
            {"device", ocr.device}
        }},
        {"pdf", {
            {"dpi", pdf.dpi},
            {"max_pages", pdf.maxPages}
        }},
        {"preprocessing", {
            {"enabled", preprocessing.enabled},
            {"grayscale", preprocessing.grayscale},
            {"denoise", preprocessing.denoise},
            {"denoise_method", denoiseMethodStr},
            {"denoise_kernel_size", preprocessing.denoiseKernelSize},
            {"denoise_sigma", preprocessing.denoiseSigma},
            {"contrast", preprocessing.contrast},
            {"contrast_method", contrastMethodStr},
            {"clahe_clip_limit", preprocessing.claheClipLimit},
            {"clahe_grid_size", preprocessing.claheGridSize},
            {"binarization", preprocessing.binarization},
            {"binarization_method", binarizationMethodStr},
            {"global_threshold", preprocessing.globalThreshold},
            {"adaptive_block_size", preprocessing.adaptiveBlockSize},
            {"adaptive_c", preprocessing.adaptiveC},
            {"deskew", preprocessing.deskew},
            {"deskew_method", deskewMethodStr},
            {"max_skew_angle_degrees", preprocessing.maxSkewAngleDegrees},
            {"min_skew_angle_threshold", preprocessing.minSkewAngleThreshold},
            {"orientation", preprocessing.orientation},
            {"sharpen", preprocessing.sharpen},
            {"sharpen_strength", preprocessing.sharpenStrength}
        }},
        {"output", {
            {"format", std::string(outputFormatToString(output.format))},
            {"include_tokens", output.includeTokens},
            {"include_lines", output.includeLines},
            {"include_blocks", output.includeBlocks},
            {"include_raw_text", output.includeRawText},
            {"include_normalized_text", output.includeNormalizedText},
            {"include_geometry", output.includeGeometry},
            {"include_timings", output.includeTimings},
            {"pretty_print", output.prettyPrint}
        }},
        {"limits", {
            {"max_file_size_mb", limits.maxFileSizeMb},
            {"max_pages", limits.maxPages},
            {"max_image_dimension", limits.maxImageDimension}
        }},
        {"debug", {
            {"enabled", debug.enabled},
            {"output_dir", debug.outputDir},
            {"save_intermediate_images", debug.saveIntermediateImages}
        }},
        {"ingestion", ingestion.toJson()}
    };
}

OcrOptions OcrConfig::toOcrOptions() const {
    OcrOptions opt;
    opt.language = ocr.language;
    opt.psm = ocr.psm;
    opt.oem = ocr.oem;
    opt.tessdataDir = ocr.tessdataDir;
    opt.charWhitelist = ocr.charWhitelist;
    opt.charBlacklist = ocr.charBlacklist;
    opt.confidenceThreshold = ocr.confidenceThreshold;
    opt.dpi = pdf.dpi > 0 ? pdf.dpi : 300;
    return opt;
}

} // namespace ocr
