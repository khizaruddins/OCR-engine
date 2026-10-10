#pragma once

#include <ocr/representation/RepresentationConfig.hpp>
#include <ocr/representation/SemanticTable.hpp>

#include <vector>

namespace ocr::representation {

class MultiPageContinuity {
public:
    static std::vector<SemanticTable> mergeContinuousTables(
        std::vector<SemanticTable> tables,
        const TableConfig& config = {}
    );

    static bool canMerge(
        const SemanticTable& a,
        const SemanticTable& b
    );
};

} // namespace ocr::representation
