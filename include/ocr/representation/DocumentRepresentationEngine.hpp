#pragma once

#include <ocr/OcrResult.hpp>
#include <ocr/Result.hpp>
#include <ocr/representation/RepresentationConfig.hpp>
#include <ocr/representation/SemanticDocument.hpp>

#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>

namespace ocr::representation {

/**
 * @brief Main engine transforming raw OCR result or raw OCR JSON into a structured, semantic SemanticDocument.
 */
class DocumentRepresentationEngine {
public:
    explicit DocumentRepresentationEngine(RepresentationConfig config = {});
    ~DocumentRepresentationEngine();

    DocumentRepresentationEngine(DocumentRepresentationEngine&&) noexcept;
    DocumentRepresentationEngine& operator=(DocumentRepresentationEngine&&) noexcept;

    DocumentRepresentationEngine(const DocumentRepresentationEngine&) = delete;
    DocumentRepresentationEngine& operator=(const DocumentRepresentationEngine&) = delete;

    [[nodiscard]] const RepresentationConfig& config() const noexcept { return config_; }
    void setConfig(RepresentationConfig config) noexcept { config_ = std::move(config); }

    /**
     * @brief Transform in-memory OcrResult to SemanticDocument.
     */
    [[nodiscard]] Result<SemanticDocument> transform(const OcrResult& ocrResult) const;

    /**
     * @brief Transform raw OCR JSON file into SemanticDocument.
     */
    [[nodiscard]] Result<SemanticDocument> transform(const std::filesystem::path& rawOcrJsonPath) const;

    /**
     * @brief Transform raw OCR JSON object into SemanticDocument.
     */
    [[nodiscard]] Result<SemanticDocument> transform(const nlohmann::json& rawOcrJson) const;

    /**
     * @brief Result containing paths of stored raw JSON and derived EDR representations.
     */
    struct StoredRepresentationPaths {
        std::filesystem::path rawJsonPath;
        std::filesystem::path edrFullJsonPath;
        std::filesystem::path edrLlmJsonPath;
        SemanticDocument semanticDocument;
    };

    /**
     * @brief Store raw OCR JSON to disk first, then use that raw JSON to create Full EDR and LLM EDR representations.
     */
    [[nodiscard]] Result<StoredRepresentationPaths> storeRawAndCreateEdr(
        const OcrResult& ocrResult,
        const std::filesystem::path& outputDir,
        const std::string& fileStem,
        bool includeSourceInLlm = false
    ) const;

    /**
     * @brief Serialize SemanticDocument to Full EDR JSON (with bounding boxes, source references, and confidence summaries).
     */
    [[nodiscard]] static nlohmann::json toFullJson(const SemanticDocument& doc);

    /**
     * @brief Serialize SemanticDocument to LLM EDR JSON (compact, LLM-ready format).
     */
    [[nodiscard]] static nlohmann::json toLlmJson(const SemanticDocument& doc, bool includeSource = false);

private:
    RepresentationConfig config_;
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ocr::representation
