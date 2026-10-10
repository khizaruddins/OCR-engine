#pragma once

#include <edmo/ocr/representation/RepresentationBlock.hpp>
#include <edmo/ocr/representation/RepresentationConfig.hpp>
#include <edmo/ocr/representation/SemanticSection.hpp>
#include <edmo/ocr/representation/SemanticTable.hpp>

#include <map>
#include <string>
#include <vector>

namespace edmo::ocr::representation {

class SectionDetector {
public:
    static std::vector<SemanticSection> detectSections(
        const std::vector<RepresentationBlock>& blocks,
        const std::vector<SemanticTable>& tables,
        const SectionConfig& config = {}
    );

    static SectionType classifySectionFromText(
        const std::string& text
    );

    static void extractGlobalEntities(
        const std::vector<SemanticSection>& sections,
        std::map<std::string, std::string>& outIdentity,
        std::map<std::string, std::string>& outInstitution,
        std::map<std::string, std::string>& outAcademic
    );
};

} // namespace edmo::ocr::representation
