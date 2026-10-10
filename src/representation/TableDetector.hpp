#pragma once

#include <edmo/ocr/OcrLine.hpp>
#include <edmo/ocr/representation/RepresentationConfig.hpp>

#include <vector>

namespace edmo::ocr::representation {

struct TableRegionCandidate {
    int startLineIndex = 0;
    int endLineIndex = 0; // inclusive
    bool hasHeader = false;
    int headerLineIndex = -1;
    float confidence = 0.0f;
    std::string tableType = "generic_table"; // "course_records", "grade_summary", "generic_table"
};

class TableDetector {
public:
    static std::vector<TableRegionCandidate> detectTableRegions(
        const std::vector<OcrLine>& lines,
        const TableConfig& config = {}
    );

    static bool isTableHeaderLine(
        const OcrLine& line
    );

    static bool isCourseRowLine(
        const OcrLine& line,
        const TableConfig& config = {}
    );

    static bool isCourseCode(
        std::string_view token
    );

    static bool isGradeToken(
        std::string_view token,
        const std::vector<std::string>& gradeVocab
    );

    static bool isNumericCreditToken(
        std::string_view token
    );
};

} // namespace edmo::ocr::representation
