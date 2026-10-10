#pragma once

#include <edmo/ocr/representation/RepresentationConfig.hpp>
#include <edmo/ocr/representation/SemanticTable.hpp>

#include <vector>

namespace edmo::ocr::representation {

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

} // namespace edmo::ocr::representation
