#pragma once

#include <edmo/ocr/Result.hpp>
#include <edmo/ocr/context/LlmContextConfig.hpp>
#include <edmo/ocr/context/LlmContextModels.hpp>
#include <edmo/ocr/context/LlmContextReport.hpp>
#include <edmo/ocr/representation/SemanticDocument.hpp>

#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>

namespace edmo::ocr::context {

/**
 * @brief Output container holding both the generated LLM Context and its audit report.
 */
struct LlmContextBuildResult {
    LlmContext context;
    LlmContextReport report;
    std::filesystem::path llmContextPath;
    std::filesystem::path reportPath;
};

/**
 * @brief Main engine transforming Full EDR into a Loss-Aware LLM Context with audit reporting.
 */
class LlmContextBuilder {
public:
    explicit LlmContextBuilder(LlmContextConfig config = {});
    ~LlmContextBuilder();

    LlmContextBuilder(LlmContextBuilder&&) noexcept;
    LlmContextBuilder& operator=(LlmContextBuilder&&) noexcept;

    LlmContextBuilder(const LlmContextBuilder&) = delete;
    LlmContextBuilder& operator=(const LlmContextBuilder&) = delete;

    [[nodiscard]] const LlmContextConfig& config() const noexcept { return config_; }
    void setConfig(LlmContextConfig config) noexcept { config_ = std::move(config); }

    /**
     * @brief Build Loss-Aware LLM Context from an in-memory SemanticDocument (Full EDR).
     */
    [[nodiscard]] Result<LlmContextBuildResult> build(
        const representation::SemanticDocument& fullEdrDoc,
        const OcrResult* optionalRawOcr = nullptr
    ) const;

    /**
     * @brief Build Loss-Aware LLM Context from Full EDR JSON object.
     */
    [[nodiscard]] Result<LlmContextBuildResult> build(
        const nlohmann::json& fullEdrJson,
        const nlohmann::json* optionalRawOcrJson = nullptr
    ) const;

    /**
     * @brief Build Loss-Aware LLM Context from Full EDR JSON file path.
     */
    [[nodiscard]] Result<LlmContextBuildResult> build(
        const std::filesystem::path& fullEdrJsonPath,
        const std::filesystem::path* optionalRawOcrJsonPath = nullptr
    ) const;

    /**
     * @brief Helper to write both llm-context.json and llm-context-report.json to disk.
     */
    static bool saveToDirectory(
        const LlmContextBuildResult& result,
        const std::filesystem::path& outputDir,
        const std::string& fileStem,
        bool prettyPrint = true
    );

private:
    LlmContextConfig config_;
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace edmo::ocr::context
