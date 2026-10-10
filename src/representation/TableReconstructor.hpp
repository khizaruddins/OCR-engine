#pragma once

#include <ocr/OcrLine.hpp>
#include <ocr/representation/RepresentationConfig.hpp>
#include <ocr/representation/SemanticTable.hpp>
#include "TableDetector.hpp"

#include <vector>

namespace ocr::representation {

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

} // namespace ocr::representation
