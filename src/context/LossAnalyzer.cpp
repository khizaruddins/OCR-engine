#include "LossAnalyzer.hpp"
#include "../representation/TextNormalizer.hpp"
#include <edmo/ocr/representation/EdrJsonSerializer.hpp>

#include <algorithm>
#include <set>
#include <sstream>

namespace edmo::ocr::context {

LlmContextReport LossAnalyzer::analyze(
    const representation::SemanticDocument& doc,
    const LlmContext& context,
    const SemanticInventory& inventory,
    const nlohmann::json* optionalRawOcrJson
) {
    LlmContextReport report;
    report.documentId = doc.metadata.id;
    report.documentType = context.documentType;
    report.isChunked = context.isChunked;
    report.chunkCount = context.isChunked ? context.chunks.size() : 1;

    // 1. Page Coverage
    report.pageCoverage.detected = doc.metadata.pageCount;
    std::set<int> presentPages(context.pagesPresent.begin(), context.pagesPresent.end());
    report.pageCoverage.represented = static_cast<int>(presentPages.size());
    for (int p = 1; p <= doc.metadata.pageCount; ++p) {
        if (presentPages.find(p) == presentPages.end()) {
            report.pageCoverage.pagesMissing.push_back(p);
        }
    }
    if (report.pageCoverage.detected > 0) {
        report.pageCoverage.coveragePercentage = std::min(100.0, (static_cast<double>(report.pageCoverage.represented) / static_cast<double>(report.pageCoverage.detected)) * 100.0);
    } else {
        report.pageCoverage.coveragePercentage = 100.0;
    }

    // 2. Section Coverage (Logical representation tracking)
    report.sectionCoverage.sectionsDetected = doc.sections.size();
    size_t representedSections = 0;
    report.sectionCoverage.sections.clear();

    for (const auto& sec : doc.sections) {
        SectionItemFate sFate;
        sFate.sourceId = sec.id;
        std::string secType = std::string(representation::sectionTypeToString(sec.type));
        if (secType == "unknown" || secType.empty()) {
            std::string idStr = sec.id;
            if (idStr.rfind("section_", 0) == 0) {
                secType = idStr.substr(8);
            } else if (idStr.rfind("sec_", 0) == 0) {
                secType = idStr.substr(4);
            } else if (!idStr.empty()) {
                secType = idStr;
            } else {
                secType = "unknown";
            }
        }
        sFate.type = secType;
        sFate.represented = false;

        // Check if section was transformed into tables (e.g. course history/academic records)
        bool hasTables = (!sec.tables.empty() || sec.type == representation::SectionType::CourseHistory) && !context.tables.empty();
        if (hasTables) {
            sFate.fate = "STRUCTURED";
            sFate.represented = true;
        } else {
            // Check if represented in context.sections
            bool foundInSections = false;
            for (const auto& ctxSec : context.sections) {
                if (ctxSec.id == sec.id || ctxSec.type == sFate.type) {
                    foundInSections = true;
                    sFate.fate = (!ctxSec.fields.empty()) ? "STRUCTURED" : "COMPACTED";
                    sFate.represented = true;
                    break;
                }
            }

            if (!foundInSections) {
                // Check if deduplicated into document header / footer
                bool allDeduplicated = true;
                bool hasBlocks = !sec.blocks.empty();
                for (const auto& b : sec.blocks) {
                    auto it = inventory.itemIndexById.find(b.id);
                    if (it != inventory.itemIndexById.end() && it->second < inventory.items.size()) {
                        if (inventory.items[it->second].fate != ItemFate::Deduplicated) {
                            allDeduplicated = false;
                        }
                    }
                }

                if (hasBlocks && allDeduplicated) {
                    sFate.fate = "DEDUPLICATED";
                    sFate.represented = true;
                } else if (hasBlocks) {
                    // Check if blocks were compacted into general/unclassified sections
                    bool anyCompacted = false;
                    for (const auto& b : sec.blocks) {
                        auto it = inventory.itemIndexById.find(b.id);
                        if (it != inventory.itemIndexById.end() && it->second < inventory.items.size()) {
                            if (inventory.items[it->second].fate != ItemFate::Unaccounted &&
                                inventory.items[it->second].fate != ItemFate::Noise) {
                                anyCompacted = true;
                                break;
                            }
                        }
                    }
                    if (anyCompacted) {
                        sFate.fate = "COMPACTED";
                        sFate.represented = true;
                    } else {
                        sFate.fate = "UNACCOUNTED_LOSS";
                        sFate.represented = false;
                    }
                } else {
                    sFate.fate = "COMPACTED";
                    sFate.represented = true;
                }
            }
        }

        if (sFate.represented) {
            representedSections++;
        }

        report.sectionCoverage.sections.push_back(std::move(sFate));
    }

    report.sectionCoverage.sectionsRepresented = representedSections;
    if (report.sectionCoverage.sectionsDetected > 0) {
        report.sectionCoverage.coveragePercentage = std::min(100.0, (static_cast<double>(representedSections) / static_cast<double>(report.sectionCoverage.sectionsDetected)) * 100.0);
    } else {
        report.sectionCoverage.coveragePercentage = 100.0;
    }

    // 3. Table Coverage
    report.tableCoverage.tablesDetected = inventory.totalTables;
    report.tableCoverage.tablesRepresented = context.tables.size();
    report.tableCoverage.rowsDetected = inventory.totalRows;
    report.tableCoverage.cellsDetected = inventory.totalCells;

    size_t repRows = 0;
    size_t repCells = 0;
    for (const auto& tbl : context.tables) {
        repRows += tbl.rows.size();
        for (const auto& r : tbl.rows) {
            repCells += r.size();
        }
    }
    report.tableCoverage.rowsRepresented = repRows;
    report.tableCoverage.cellsRepresented = repCells;

    if (report.tableCoverage.tablesDetected > 0) {
        report.tableCoverage.tableCoveragePercentage = std::min(100.0, (static_cast<double>(report.tableCoverage.tablesRepresented) / static_cast<double>(report.tableCoverage.tablesDetected)) * 100.0);
    } else {
        report.tableCoverage.tableCoveragePercentage = 100.0;
    }

    if (report.tableCoverage.rowsDetected > 0) {
        report.tableCoverage.rowCoveragePercentage = std::min(100.0, (static_cast<double>(report.tableCoverage.rowsRepresented) / static_cast<double>(report.tableCoverage.rowsDetected)) * 100.0);
    } else {
        report.tableCoverage.rowCoveragePercentage = 100.0;
    }

    if (report.tableCoverage.cellsDetected > 0) {
        report.tableCoverage.cellCoveragePercentage = std::min(100.0, (static_cast<double>(report.tableCoverage.cellsRepresented) / static_cast<double>(report.tableCoverage.cellsDetected)) * 100.0);
    } else {
        report.tableCoverage.cellCoveragePercentage = 100.0;
    }

    // 4. Confidence Preservation (Accurate source tracking vs output annotations)
    report.confidencePreservation.sourceLowConfidenceItems = inventory.sourceLowConfidenceIds.size();
    size_t preservedLowConfSourceItems = 0;
    for (const auto& srcId : inventory.sourceLowConfidenceIds) {
        auto it = inventory.itemIndexById.find(srcId);
        if (it != inventory.itemIndexById.end() && it->second < inventory.items.size()) {
            const auto& item = inventory.items[it->second];
            if (item.fate != ItemFate::Unaccounted && item.fate != ItemFate::Noise) {
                preservedLowConfSourceItems++;
            }
        }
    }
    report.confidencePreservation.sourceItemsWithPreservedEvidence = preservedLowConfSourceItems;

    if (report.confidencePreservation.sourceLowConfidenceItems > 0) {
        report.confidencePreservation.preservationPercentage = std::min(100.0, (static_cast<double>(preservedLowConfSourceItems) / static_cast<double>(report.confidencePreservation.sourceLowConfidenceItems)) * 100.0);
    } else {
        report.confidencePreservation.preservationPercentage = 100.0;
    }

    size_t totalOutputAnnotations = 0;
    for (const auto& tbl : context.tables) {
        totalOutputAnnotations += tbl.lowConfidenceCells.size();
    }
    totalOutputAnnotations += context.warnings.size();
    report.confidencePreservation.outputConfidenceAnnotations = totalOutputAnnotations;

    // 5. Item-Fate Accounting (Sum must equal totalSourceItems)
    report.lossAnalysis.totalSourceItems = inventory.items.size();
    for (const auto& item : inventory.items) {
        switch (item.fate) {
            case ItemFate::Preserved: report.lossAnalysis.preservedItems++; break;
            case ItemFate::Compacted: report.lossAnalysis.compactedItems++; break;
            case ItemFate::Structured: report.lossAnalysis.structuredItems++; break;
            case ItemFate::Deduplicated: report.lossAnalysis.deduplicatedItems++; break;
            case ItemFate::Noise: report.lossAnalysis.noiseItems++; break;
            case ItemFate::Metadata: report.lossAnalysis.metadataItems++; break;
            case ItemFate::Chunked: report.lossAnalysis.chunkedItems++; break;
            case ItemFate::Unaccounted: default: report.lossAnalysis.unaccountedLossItems++; break;
        }
    }

    // 6. Compression & Meaningful Text
    std::string contextDump = context.toJson(false).dump();
    report.compression.llmContextBytes = contextDump.size();
    report.compression.llmContextCharacters = contextDump.size();
    report.compression.estimatedLlmTokens = contextDump.size() / 4;

    if (optionalRawOcrJson != nullptr) {
        std::string rawDump = optionalRawOcrJson->dump();
        report.compression.rawBytes = rawDump.size();
        report.compression.rawCharacters = rawDump.size();
        report.compression.estimatedRawTokens = rawDump.size() / 4;
    } else if (doc.compression.rawBytes > 0) {
        report.compression.rawBytes = doc.compression.rawBytes;
        report.compression.rawCharacters = doc.compression.rawCharacters;
        report.compression.estimatedRawTokens = doc.compression.estimatedRawTokens;
    }

    if (doc.compression.edrFullBytes > 0) {
        report.compression.edrFullBytes = doc.compression.edrFullBytes;
        report.compression.edrFullCharacters = doc.compression.edrFullCharacters;
        report.compression.estimatedFullTokens = doc.compression.estimatedFullTokens;
    } else {
        std::string edrDump = representation::EdrJsonSerializer::toFullJson(doc).dump();
        report.compression.edrFullBytes = edrDump.size();
        report.compression.edrFullCharacters = edrDump.size();
        report.compression.estimatedFullTokens = edrDump.size() / 4;
    }

    if (report.compression.rawBytes > 0 && report.compression.edrFullBytes > 0) {
        report.compression.rawToFullRatio = static_cast<double>(report.compression.rawBytes) / static_cast<double>(report.compression.edrFullBytes);
    }

    if (report.compression.edrFullBytes > 0 && report.compression.llmContextBytes > 0) {
        report.compression.fullToLlmRatio = static_cast<double>(report.compression.edrFullBytes) / static_cast<double>(report.compression.llmContextBytes);
    }

    if (report.compression.rawBytes > 0 && report.compression.llmContextBytes > 0) {
        report.compression.rawToLlmRatio = static_cast<double>(report.compression.rawBytes) / static_cast<double>(report.compression.llmContextBytes);
    }

    // 7. Text Coverage Breakdown
    report.textCoverage.sourceMeaningfulText = inventory.totalMeaningfulCharacters;
    size_t representedChars = 0;
    std::string upperDump = representation::TextNormalizer::toUpper(contextDump);

    for (const auto& item : inventory.items) {
        if (item.text.empty()) continue;

        if (item.fate != ItemFate::Noise && item.fate != ItemFate::Unaccounted) {
            std::string upperText = representation::TextNormalizer::toUpper(item.text);
            if (upperDump.find(upperText) != std::string::npos) {
                representedChars += item.text.size();
            } else {
                std::istringstream iss(upperText);
                std::string tok;
                size_t matchedTokChars = 0;
                while (iss >> tok) {
                    if (tok.size() >= 2 && upperDump.find(tok) != std::string::npos) {
                        matchedTokChars += tok.size();
                    }
                }
                if (matchedTokChars > 0 && matchedTokChars >= (upperText.size() / 3)) {
                    representedChars += item.text.size();
                } else if (item.fate == ItemFate::Deduplicated) {
                    report.textCoverage.unrepresentedText.deduplicated += item.text.size();
                } else if (item.fate == ItemFate::Metadata) {
                    report.textCoverage.unrepresentedText.metadata += item.text.size();
                }
            }
        } else if (item.fate == ItemFate::Noise) {
            report.textCoverage.unrepresentedText.noise += item.text.size();
        } else if (item.fate == ItemFate::Unaccounted) {
            report.textCoverage.unrepresentedText.unaccounted += item.text.size();
        }
    }

    report.textCoverage.representedMeaningfulText = representedChars;
    if (inventory.totalMeaningfulCharacters > 0) {
        report.textCoverage.coveragePercentage = std::min(100.0, (static_cast<double>(representedChars) / static_cast<double>(inventory.totalMeaningfulCharacters)) * 100.0);
    } else {
        report.textCoverage.coveragePercentage = 100.0;
    }
    report.compression.meaningfulTextCoveragePercentage = report.textCoverage.coveragePercentage;

    // 8. Context Quality Score
    double pageWeight = 0.25;
    double rowWeight = 0.35;
    double textWeight = 0.20;
    double confWeight = 0.20;

    double quality = (pageWeight * report.pageCoverage.coveragePercentage) +
                     (rowWeight * report.tableCoverage.rowCoveragePercentage) +
                     (textWeight * report.textCoverage.coveragePercentage) +
                     (confWeight * report.confidencePreservation.preservationPercentage);

    if (report.lossAnalysis.unaccountedLossItems > 0) {
        quality = std::max(0.0, quality - 50.0);
        report.warnings.push_back("Audit detected " + std::to_string(report.lossAnalysis.unaccountedLossItems) + " unaccounted loss item(s).");
    }

    report.contextQualityScore = std::min(100.0, std::max(0.0, quality));

    // 9. Status Determination
    if (report.lossAnalysis.unaccountedLossItems > 0) {
        report.status = "ERROR_UNACCOUNTED_LOSS";
    } else if (report.sectionCoverage.coveragePercentage < 100.0) {
        report.status = "WARNING_SECTION_COVERAGE";
    } else if (report.lossAnalysis.noiseItems > 0 || report.lossAnalysis.metadataItems > 0) {
        report.status = "VERIFIED_WITH_EXPLICIT_EXCLUSIONS";
    } else {
        report.status = "VERIFIED_ZERO_UNACCOUNTED_LOSS";
    }

    return report;
}

} // namespace edmo::ocr::context
