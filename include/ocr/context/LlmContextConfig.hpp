#pragma once

#include <cstddef>
#include <string>

namespace edmo::ocr::context {

/**
 * @brief Context output formatting mode.
 */
enum class ContextMode {
    Compact,    ///< Ultra-compact without bounding boxes (ideal for LLM prompt payload)
    Traceable   ///< Includes region-level page and source references for traceability
};

/**
 * @brief Confidence handling thresholds for context builder.
 */
struct ConfidenceThresholds {
    float preserveBelow = 0.80f;  ///< Explicitly include confidence score if token < preserveBelow
    float criticalBelow = 0.60f;  ///< Flag as low_confidence warning if token < criticalBelow
};

/**
 * @brief Deduplication configuration for repeated document-level artifacts.
 */
struct DeduplicationConfig {
    bool deduplicateHeaders = true;     ///< Deduplicate identical institution/document header banners
    bool deduplicateFooters = true;     ///< Deduplicate identical document footer notices
    bool deduplicatePageNumbers = true; ///< Normalize page number patterns into metadata
    size_t minOccurrences = 2;          ///< Minimum occurrences across pages to trigger deduplication
};

/**
 * @brief Semantic chunking configuration when document exceeds token budget.
 */
struct ChunkingConfig {
    bool enabled = true;                    ///< Enable semantic chunking if budget exceeded
    size_t maxRowsPerTableChunk = 50;       ///< Maximum table rows per individual chunk
    bool preserveRowIntegrity = true;       ///< STRICT: Never split table rows across chunks
    bool preserveCellIntegrity = true;      ///< STRICT: Never split individual cells across chunks
    bool includeGlobalContext = true;       ///< Attach document-level metadata to each chunk
};

/**
 * @brief Configuration for the Loss-Aware LLM Context Builder.
 */
struct LlmContextConfig {
    size_t tokenBudget = 12000;             ///< Configurable LLM token budget
    ContextMode mode = ContextMode::Compact;///< Compact or Traceable mode
    ConfidenceThresholds confidence;        ///< Confidence retention thresholds
    DeduplicationConfig deduplication;      ///< Safe deduplication parameters
    ChunkingConfig chunking;                ///< Semantic chunking rules
    bool includeCoverageReport = true;      ///< Generate llm-context-report.json
    bool prettyPrint = true;                ///< Pretty-print JSON outputs
};

} // namespace edmo::ocr::context
