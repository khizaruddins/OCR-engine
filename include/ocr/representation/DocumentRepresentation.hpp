#pragma once

#include <edmo/ocr/BoundingBox.hpp>
#include <edmo/ocr/representation/ConfidenceSummary.hpp>
#include <edmo/ocr/representation/SourceReference.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace edmo::ocr::representation {

/**
 * @brief Categorization of document type.
 */
enum class DocumentType {
    Transcript,
    Marksheet,
    Degree,
    Diploma,
    Resume,
    SOP,
    LOR,
    ApplicationForm,
    TransferCredit,
    Generic,
    Other,
    Unknown
};

[[nodiscard]] constexpr std::string_view documentTypeToString(DocumentType type) noexcept {
    switch (type) {
        case DocumentType::Transcript: return "transcript";
        case DocumentType::Marksheet: return "marksheet";
        case DocumentType::Degree: return "degree";
        case DocumentType::Diploma: return "diploma";
        case DocumentType::Resume: return "resume";
        case DocumentType::SOP: return "sop";
        case DocumentType::LOR: return "lor";
        case DocumentType::ApplicationForm: return "application_form";
        case DocumentType::TransferCredit: return "transfer_credit";
        case DocumentType::Generic: return "generic";
        case DocumentType::Other: return "other";
        case DocumentType::Unknown: default: return "unknown";
    }
}

[[nodiscard]] inline DocumentType stringToDocumentType(std::string_view str) noexcept {
    if (str == "transcript") return DocumentType::Transcript;
    if (str == "marksheet") return DocumentType::Marksheet;
    if (str == "degree") return DocumentType::Degree;
    if (str == "diploma") return DocumentType::Diploma;
    if (str == "resume") return DocumentType::Resume;
    if (str == "sop") return DocumentType::SOP;
    if (str == "lor") return DocumentType::LOR;
    if (str == "application_form") return DocumentType::ApplicationForm;
    if (str == "transfer_credit") return DocumentType::TransferCredit;
    if (str == "generic") return DocumentType::Generic;
    if (str == "other") return DocumentType::Other;
    return DocumentType::Unknown;
}

/**
 * @brief Extensible semantic section categories.
 */
enum class SectionType {
    Header,
    StudentInformation,
    InstitutionInformation,
    AcademicInformation,
    CourseHistory,
    AcademicSummary,
    DegreeInformation,
    GradingScale,
    Footer,
    Signature,
    Other,
    Unknown
};

[[nodiscard]] constexpr std::string_view sectionTypeToString(SectionType type) noexcept {
    switch (type) {
        case SectionType::Header: return "header";
        case SectionType::StudentInformation: return "student_information";
        case SectionType::InstitutionInformation: return "institution_information";
        case SectionType::AcademicInformation: return "academic_information";
        case SectionType::CourseHistory: return "course_history";
        case SectionType::AcademicSummary: return "academic_summary";
        case SectionType::DegreeInformation: return "degree_information";
        case SectionType::GradingScale: return "grading_scale";
        case SectionType::Footer: return "footer";
        case SectionType::Signature: return "signature";
        case SectionType::Other: return "other";
        case SectionType::Unknown: default: return "unknown";
    }
}

[[nodiscard]] inline SectionType stringToSectionType(std::string_view str) noexcept {
    if (str == "header") return SectionType::Header;
    if (str == "student_information" || str == "student_info" || str == "student") return SectionType::StudentInformation;
    if (str == "institution_information" || str == "institution_info" || str == "institution") return SectionType::InstitutionInformation;
    if (str == "academic_information" || str == "academic_info" || str == "academic") return SectionType::AcademicInformation;
    if (str == "course_history" || str == "course_records" || str == "academic_record" || str == "academic_records") return SectionType::CourseHistory;
    if (str == "academic_summary" || str == "summary" || str == "totals") return SectionType::AcademicSummary;
    if (str == "degree_information" || str == "degree") return SectionType::DegreeInformation;
    if (str == "grading_scale" || str == "grading_system") return SectionType::GradingScale;
    if (str == "footer") return SectionType::Footer;
    if (str == "signature") return SectionType::Signature;
    if (str == "other") return SectionType::Other;
    return SectionType::Unknown;
}

/**
 * @brief Representation block classification.
 */
enum class BlockType {
    Heading,
    Paragraph,
    LabelValue,
    Table,
    Header,
    Footer,
    List,
    Unknown
};

[[nodiscard]] constexpr std::string_view blockTypeToString(BlockType type) noexcept {
    switch (type) {
        case BlockType::Heading: return "heading";
        case BlockType::Paragraph: return "paragraph";
        case BlockType::LabelValue: return "label_value";
        case BlockType::Table: return "table";
        case BlockType::Header: return "header";
        case BlockType::Footer: return "footer";
        case BlockType::List: return "list";
        case BlockType::Unknown: default: return "unknown";
    }
}

/**
 * @brief Spatial region on a document page.
 */
struct SpatialRegion {
    std::string id;
    int pageNumber = 1;
    BoundingBox bbox;
    std::string type = "unknown"; // header, footer, body, sidebar, table, form, unknown
    std::vector<std::string> blockIds;
    float confidence = 0.0f;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"id", id},
            {"page_number", pageNumber},
            {"bbox", bbox.toJson()},
            {"type", type},
            {"block_ids", blockIds},
            {"confidence", confidence}
        };
    }
};

/**
 * @brief Warning annotation for low confidence or suspicious regions.
 */
struct LowConfidenceWarning {
    std::string type = "low_confidence"; // low_confidence, possible_ocr_confusion, missing_field
    int page = 1;
    std::string region;
    std::string field;
    float confidence = 0.0f;
    std::string message;

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j{
            {"type", type},
            {"page", page},
            {"region", region},
            {"confidence", confidence}
        };
        if (!field.empty()) j["field"] = field;
        if (!message.empty()) j["message"] = message;
        return j;
    }
};

/**
 * @brief Detailed compression and token metrics.
 */
struct CompressionMetrics {
    size_t rawBytes = 0;
    size_t rawCharacters = 0;
    size_t edrFullBytes = 0;
    size_t edrFullCharacters = 0;
    size_t edrLlmBytes = 0;
    size_t edrLlmCharacters = 0;

    double rawToFullRatio = 1.0;
    double rawToLlmRatio = 1.0;

    size_t estimatedRawTokens = 0;
    size_t estimatedFullTokens = 0;
    size_t estimatedLlmTokens = 0;

    double textPreservationPercentage = 100.0;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"raw_bytes", rawBytes},
            {"raw_characters", rawCharacters},
            {"edr_full_bytes", edrFullBytes},
            {"edr_full_characters", edrFullCharacters},
            {"edr_llm_bytes", edrLlmBytes},
            {"edr_llm_characters", edrLlmCharacters},
            {"raw_to_full_ratio", rawToFullRatio},
            {"raw_to_llm_ratio", rawToLlmRatio},
            {"estimated_raw_tokens", estimatedRawTokens},
            {"estimated_full_tokens", estimatedFullTokens},
            {"estimated_llm_tokens", estimatedLlmTokens},
            {"text_preservation_percentage", textPreservationPercentage}
        };
    }
};

/**
 * @brief Semantic chunk model for future modular LLM context feeding.
 */
struct SemanticChunk {
    std::string id;
    std::string type;
    int pageStart = 1;
    int pageEnd = 1;
    std::string content;
    int estimatedTokens = 0;
    ConfidenceSummary confidence;

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"id", id},
            {"type", type},
            {"page_start", pageStart},
            {"page_end", pageEnd},
            {"content", content},
            {"estimated_tokens", estimatedTokens},
            {"confidence", confidence.toJson()}
        };
    }
};

} // namespace edmo::ocr::representation
