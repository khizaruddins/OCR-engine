#pragma once

#include <edmo/ocr/representation/ConfidenceSummary.hpp>
#include <edmo/ocr/representation/DocumentRepresentation.hpp>
#include <edmo/ocr/representation/RepresentationBlock.hpp>
#include <edmo/ocr/representation/RepresentationConfig.hpp>
#include <edmo/ocr/representation/SemanticDocument.hpp>
#include <edmo/ocr/representation/SemanticSection.hpp>
#include <edmo/ocr/representation/SemanticTable.hpp>

#include <vector>

namespace edmo::ocr::representation {

class ConfidenceAggregator {
public:
    static ConfidenceSummary aggregateDocumentConfidence(
        const SemanticDocument& doc
    );

    static std::vector<LowConfidenceWarning> findLowConfidenceRegions(
        const SemanticDocument& doc,
        float threshold = 0.70f
    );
};

} // namespace edmo::ocr::representation
