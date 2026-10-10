#include "ConfidenceAggregator.hpp"

namespace edmo::ocr::representation {

ConfidenceSummary ConfidenceAggregator::aggregateDocumentConfidence(
    const SemanticDocument& doc
) {
    ConfidenceSummary summary;
    for (const auto& page : doc.pages) {
        summary.merge(page.confidence);
    }
    return summary;
}

std::vector<LowConfidenceWarning> ConfidenceAggregator::findLowConfidenceRegions(
    const SemanticDocument& doc,
    float threshold
) {
    std::vector<LowConfidenceWarning> warnings;

    // Check tables
    for (const auto& tbl : doc.tables) {
        if (tbl.confidence.tokenCount > 0 && tbl.confidence.mean < threshold) {
            LowConfidenceWarning w;
            w.type = "low_confidence";
            w.page = tbl.pageStart;
            w.region = tbl.tableType;
            w.confidence = tbl.confidence.mean;
            w.message = "Table mean confidence (" + std::to_string(tbl.confidence.mean) + ") is below threshold";
            warnings.push_back(std::move(w));
        }

        for (const auto& r : tbl.rows) {
            if (r.confidence < threshold && r.confidence > 0.0f) {
                LowConfidenceWarning rw;
                rw.type = "low_confidence_row";
                rw.page = r.source.pageNumber;
                rw.region = tbl.tableType;
                rw.confidence = r.confidence;
                rw.message = "Course row has low OCR confidence";
                warnings.push_back(std::move(rw));
            }
        }
    }

    // Check sections
    for (const auto& sec : doc.sections) {
        if (sec.confidenceSummary.tokenCount > 0 && sec.confidenceSummary.mean < threshold) {
            LowConfidenceWarning sw;
            sw.type = "low_confidence_section";
            sw.page = sec.pageStart;
            sw.region = sectionTypeToString(sec.type);
            sw.confidence = sec.confidenceSummary.mean;
            sw.message = "Section confidence is below threshold";
            warnings.push_back(std::move(sw));
        }
    }

    return warnings;
}

} // namespace edmo::ocr::representation
