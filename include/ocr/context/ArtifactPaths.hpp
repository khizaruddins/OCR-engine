#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ocr::context {

/**
 * @brief Centralized artifact naming, directory resolution, and path sanitization.
 */
class ArtifactPaths {
public:
    static constexpr std::string_view kRawOcrFilename = "raw_ocr.json";
    static constexpr std::string_view kEdrFullFilename = "edr_full.json";
    static constexpr std::string_view kLlmContextFilename = "llm_context.json";
    static constexpr std::string_view kLlmContextReportFilename = "llm_context_report.json";

    /**
     * @brief Sanitize document name into a clean, safe directory identifier.
     *        Strips file extensions and legacy pipeline suffixes (_raw_ocr, _edr_full, etc.)
     *        Replaces spaces and unsafe characters with underscores.
     */
    [[nodiscard]] static std::string sanitizeDocumentName(std::string_view filename) {
        std::string stem(filename);

        // Remove path prefix if given full path
        auto lastSlash = stem.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            stem = stem.substr(lastSlash + 1);
        }

        // Remove known file extensions
        for (const std::string_view ext : {".pdf", ".png", ".jpg", ".jpeg", ".tiff", ".tif", ".bmp", ".webp", ".json"}) {
            if (stem.size() >= ext.size() &&
                std::equal(ext.rbegin(), ext.rend(), stem.rbegin(),
                           [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
                stem = stem.substr(0, stem.size() - ext.size());
                break;
            }
        }

        // Strip legacy pipeline suffixes to prevent recursive composition
        bool stripped = true;
        while (stripped) {
            stripped = false;
            for (const std::string_view sfx : {
                "_raw_ocr", "_edr_full", "_edr_llm", "_llm_context", "_llm_context_report",
                "-raw_ocr", "-edr_full", "-edr_llm", "-llm_context", "-llm_context_report",
                "_raw", "-raw", "_llm", "-llm"
            }) {
                if (stem.size() >= sfx.size() &&
                    stem.compare(stem.size() - sfx.size(), sfx.size(), sfx) == 0) {
                    stem = stem.substr(0, stem.size() - sfx.size());
                    stripped = true;
                    break;
                }
            }
        }

        // Replace whitespace and unsafe chars with underscores
        std::string result;
        result.reserve(stem.size());
        char lastChar = '\0';

        for (char c : stem) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '.') {
                result.push_back(c);
                lastChar = c;
            } else {
                if (lastChar != '_') {
                    result.push_back('_');
                    lastChar = '_';
                }
            }
        }

        // Trim leading and trailing underscores or hyphens
        while (!result.empty() && (result.front() == '_' || result.front() == '-')) {
            result.erase(result.begin());
        }
        while (!result.empty() && (result.back() == '_' || result.back() == '-')) {
            result.pop_back();
        }

        if (result.empty()) {
            return "document";
        }

        return result;
    }

    /**
     * @brief Resolve the document output folder: <baseOutputDir>/<sanitizedDocumentName>
     */
    [[nodiscard]] static std::filesystem::path getDocumentDirectory(
        const std::filesystem::path& baseOutputDir,
        std::string_view documentFilename
    ) {
        std::string docName = sanitizeDocumentName(documentFilename);
        // If baseOutputDir already ends with the document name, don't nest it twice
        if (baseOutputDir.filename().string() == docName) {
            return baseOutputDir;
        }
        return baseOutputDir / docName;
    }

    [[nodiscard]] static std::filesystem::path getDocumentDirectory(
        const std::filesystem::path& baseOutputDir,
        const std::filesystem::path& inputFilePath
    ) {
        std::string filename = inputFilePath.filename().string();
        std::string stem = inputFilePath.stem().string();

        // If the filename is a generic artifact name (e.g. raw_ocr.json), extract from parent directory
        if ((stem == "raw_ocr" || stem == "edr_full" || stem == "edr_llm" ||
             stem == "llm_context" || stem == "llm_context_report" || stem == "raw" || stem == "ocr") &&
            inputFilePath.has_parent_path()) {
            std::string parentName = inputFilePath.parent_path().filename().string();
            if (!parentName.empty() && parentName != "." && parentName != "documents" &&
                parentName != "output" && parentName != "raw_json") {
                return getDocumentDirectory(baseOutputDir, std::string_view(parentName));
            }
        }

        return getDocumentDirectory(baseOutputDir, std::string_view(filename));
    }

    [[nodiscard]] static std::filesystem::path rawOcrPath(const std::filesystem::path& docDir) {
        return docDir / kRawOcrFilename;
    }

    [[nodiscard]] static std::filesystem::path edrFullPath(const std::filesystem::path& docDir) {
        return docDir / kEdrFullFilename;
    }

    [[nodiscard]] static std::filesystem::path llmContextPath(const std::filesystem::path& docDir) {
        return docDir / kLlmContextFilename;
    }

    [[nodiscard]] static std::filesystem::path llmContextReportPath(const std::filesystem::path& docDir) {
        return docDir / kLlmContextReportFilename;
    }

    /**
     * @brief Check if a file extension corresponds to a supported source document format.
     */
    [[nodiscard]] static bool isSupportedDocumentExtension(std::string_view ext) {
        std::string lowerExt;
        lowerExt.reserve(ext.size());
        for (char c : ext) lowerExt.push_back(static_cast<char>(std::tolower(c)));
        if (!lowerExt.empty() && lowerExt.front() != '.') {
            lowerExt = "." + lowerExt;
        }

        return lowerExt == ".pdf" || lowerExt == ".png" || lowerExt == ".jpg" ||
               lowerExt == ".jpeg" || lowerExt == ".tiff" || lowerExt == ".tif" ||
               lowerExt == ".bmp" || lowerExt == ".webp";
    }
};

} // namespace ocr::context
