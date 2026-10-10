#pragma once

#include <edmo/ocr/OcrLine.hpp>
#include <edmo/ocr/representation/RepresentationConfig.hpp>
#include <edmo/ocr/representation/SemanticTable.hpp>
#include "TableDetector.hpp"

#include <vector>

namespace edmo::ocr::representation {

class TableReconstructor {
public:
    static SemanticTable reconstructTable(
        const std::vector<OcrLine>& lines,
        const TableRegionCandidate& candidate,
        int pageNumber,
        int tableIdIndex,
        const TableConfig& config = {}
    );

    static SemanticRow parseCourseRow(
        const OcrLine& line,
        int pageNumber,
        const TableConfig& config = {}
    );

    static std::vector<SemanticColumn> buildCanonicalColumns(
        const std::vector<SemanticRow>& rows,
        const OcrLine* headerLine
    );
};

} // namespace edmo::ocr::representation
