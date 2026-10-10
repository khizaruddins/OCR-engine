#pragma once

#include <ocr/representation/ConfidenceSummary.hpp>
#include <ocr/representation/DocumentRepresentation.hpp>
#include <ocr/representation/RepresentationBlock.hpp>
#include <ocr/representation/RepresentationConfig.hpp>
#include <ocr/representation/SemanticDocument.hpp>
#include <ocr/representation/SemanticSection.hpp>
#include <ocr/representation/SemanticTable.hpp>

#include <vector>

namespace ocr::representation {

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

} // namespace ocr::representation
