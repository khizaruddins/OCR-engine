#include "SectionDetector.hpp"
#include "TextNormalizer.hpp"

#include <algorithm>

namespace edmo::ocr::representation {

SectionType SectionDetector::classifySectionFromText(const std::string& text) {
    std::string upper = TextNormalizer::toUpper(text);

    if (TextNormalizer::icontains(upper, "STUDENT INFORMATION") ||
        TextNormalizer::icontains(upper, "STUDENT RECORD") ||
        TextNormalizer::icontains(upper, "STUDENT ID") ||
        TextNormalizer::startsWith(upper, "NAME :") ||
        TextNormalizer::startsWith(upper, "NAME:")) {
        return SectionType::StudentInformation;
    }

    if (TextNormalizer::icontains(upper, "DEGREE :") ||
        TextNormalizer::icontains(upper, "DEGREE:") ||
        TextNormalizer::icontains(upper, "MAJOR") ||
        TextNormalizer::icontains(upper, "PROGRAM :") ||
        TextNormalizer::icontains(upper, "PROGRAM:") ||
        TextNormalizer::icontains(upper, "SCHOOL :") ||
        TextNormalizer::icontains(upper, "SCHOOL:") ||
        TextNormalizer::icontains(upper, "DEGREES AWARDED") ||
        TextNormalizer::icontains(upper, "PROGRAM OF STUDY")) {
        return SectionType::AcademicInformation;
    }

    if (TextNormalizer::icontains(upper, "ACADEMIC RECORD") ||
        TextNormalizer::icontains(upper, "COURSE HISTORY") ||
        TextNormalizer::startsWith(upper, "SPRING 20") ||
        TextNormalizer::startsWith(upper, "FALL 20") ||
        TextNormalizer::startsWith(upper, "SUMMER 20") ||
        TextNormalizer::startsWith(upper, "WINTER 20")) {
        return SectionType::CourseHistory;
    }

    if (TextNormalizer::icontains(upper, "TERM GPA") ||
        TextNormalizer::icontains(upper, "CUM GPA") ||
        TextNormalizer::icontains(upper, "CUMGPA") ||
        TextNormalizer::icontains(upper, "TOTAL CREDITS EARNED") ||
        TextNormalizer::icontains(upper, "TOTALS:") ||
        TextNormalizer::icontains(upper, "TRANSCRIPT TOTALS") ||
        TextNormalizer::icontains(upper, "END OF RECORD")) {
        return SectionType::AcademicSummary;
    }

    if (TextNormalizer::icontains(upper, "COLLEGE") ||
        TextNormalizer::icontains(upper, "UNIVERSITY") ||
        TextNormalizer::icontains(upper, "OFFICE OF STUDENT SERVICES") ||
        TextNormalizer::icontains(upper, "REGISTRAR")) {
        return SectionType::InstitutionInformation;
    }

    return SectionType::Unknown;
}

std::vector<SemanticSection> SectionDetector::detectSections(
    const std::vector<RepresentationBlock>& blocks,
    const std::vector<SemanticTable>& tables,
    const SectionConfig& config
) {
    if (!config.enabled) return {};

    std::vector<SemanticSection> sections;

    // Create primary sections
    SemanticSection studentSec;
    studentSec.id = "section_student_info";
    studentSec.title = "Student Information";
    studentSec.type = SectionType::StudentInformation;

    SemanticSection instSec;
    instSec.id = "section_institution_info";
    instSec.title = "Institution Information";
    instSec.type = SectionType::InstitutionInformation;

    SemanticSection academicSec;
    academicSec.id = "section_academic_info";
    academicSec.title = "Academic Information";
    academicSec.type = SectionType::AcademicInformation;

    SemanticSection courseSec;
    courseSec.id = "section_course_history";
    courseSec.title = "Course History";
    courseSec.type = SectionType::CourseHistory;

    SemanticSection summarySec;
    summarySec.id = "section_academic_summary";
    summarySec.title = "Academic Summary";
    summarySec.type = SectionType::AcademicSummary;

    SemanticSection otherSec;
    otherSec.id = "section_other";
    otherSec.title = "General Content";
    otherSec.type = SectionType::Other;

    for (const auto& block : blocks) {
        std::string upperText = TextNormalizer::toUpper(block.text);
        std::string upperLabel = TextNormalizer::toUpper(block.label);

        if (block.type == BlockType::LabelValue) {
            std::string cleanVal = TextNormalizer::trim(block.value);

            if (upperLabel == "NAME" || upperLabel == "STUDENT NAME") {
                studentSec.fields["student_name"] = cleanVal;
                studentSec.blocks.push_back(block);
                studentSec.confidenceSummary.merge(block.confidence);
            } else if (upperLabel == "STUDENT ID" || upperLabel == "ID" || upperLabel == "STUDENT NO" || upperLabel == "ID NUMBER") {
                studentSec.fields["student_id"] = cleanVal;
                studentSec.blocks.push_back(block);
                studentSec.confidenceSummary.merge(block.confidence);
            } else if (upperLabel == "BIRTHDATE" || upperLabel == "DATE OF BIRTH" || upperLabel == "DOB") {
                studentSec.fields["birthdate"] = cleanVal;
                studentSec.blocks.push_back(block);
                studentSec.confidenceSummary.merge(block.confidence);
            } else if (upperLabel == "SCHOOL" || upperLabel == "COLLEGE") {
                academicSec.fields["school"] = cleanVal;
                academicSec.blocks.push_back(block);
                academicSec.confidenceSummary.merge(block.confidence);
            } else if (upperLabel == "DEPARTMENT" || upperLabel == "FACULTY") {
                academicSec.fields["department"] = cleanVal;
                academicSec.blocks.push_back(block);
                academicSec.confidenceSummary.merge(block.confidence);
            } else if (upperLabel == "DEGREE" || (TextNormalizer::icontains(upperLabel, "DEGREE") && !TextNormalizer::icontains(upperLabel, "MAJOR"))) {
                academicSec.fields["degree"] = cleanVal;
                academicSec.blocks.push_back(block);
                academicSec.confidenceSummary.merge(block.confidence);
            } else if (upperLabel == "MAJOR" ||
                       TextNormalizer::icontains(upperLabel, "MAJOR") ||
                       upperLabel == "PROGRAM" ||
                       TextNormalizer::icontains(upperLabel, "PROGRAM / MAJOR") ||
                       TextNormalizer::icontains(upperLabel, "PROGRAM/MAJOR") ||
                       TextNormalizer::icontains(upperLabel, "BACHELORS MAJOR") ||
                       TextNormalizer::icontains(upperLabel, "BACHELOR'S MAJOR") ||
                       TextNormalizer::icontains(upperLabel, "UNDERGRADUATE MAJOR") ||
                       TextNormalizer::icontains(upperLabel, "INTENDED MAJOR") ||
                       TextNormalizer::icontains(upperLabel, "FIELD OF STUDY") ||
                       TextNormalizer::icontains(upperLabel, "CONCENTRATION")) {
                academicSec.fields["major"] = cleanVal;
                academicSec.blocks.push_back(block);
                academicSec.confidenceSummary.merge(block.confidence);
            } else if (upperLabel == "MINOR" || TextNormalizer::icontains(upperLabel, "MINOR")) {
                academicSec.fields["minor"] = cleanVal;
                academicSec.blocks.push_back(block);
                academicSec.confidenceSummary.merge(block.confidence);
            } else if (TextNormalizer::icontains(upperLabel, "GPA")) {
                summarySec.fields["gpa"] = cleanVal;
                summarySec.blocks.push_back(block);
                summarySec.confidenceSummary.merge(block.confidence);
            } else {
                otherSec.blocks.push_back(block);
                otherSec.confidenceSummary.merge(block.confidence);
            }
        } else {
            // Non label-value blocks
            if (TextNormalizer::icontains(upperText, "BOSTON COLLEGE") ||
                TextNormalizer::icontains(upperText, "UNIVERSITY OF OXFORD") ||
                TextNormalizer::icontains(upperText, "OFFICE OF STUDENT SERVICES")) {
                if (!instSec.fields.count("name")) {
                    if (TextNormalizer::icontains(upperText, "BOSTON COLLEGE")) {
                        instSec.fields["name"] = "Boston College";
                    } else if (TextNormalizer::icontains(upperText, "UNIVERSITY OF OXFORD")) {
                        instSec.fields["name"] = "University of Oxford";
                    } else {
                        instSec.fields["name"] = TextNormalizer::trim(block.text);
                    }
                }
                instSec.blocks.push_back(block);
                instSec.confidenceSummary.merge(block.confidence);
            } else if (TextNormalizer::icontains(upperText, "TOTAL CREDITS EARNED") ||
                       TextNormalizer::icontains(upperText, "CUM GPA") ||
                       TextNormalizer::icontains(upperText, "CUMGPA")) {
                summarySec.blocks.push_back(block);
                summarySec.confidenceSummary.merge(block.confidence);

                // Extract summary numbers if found
                if (TextNormalizer::icontains(upperText, "TOTAL CREDITS EARNED:")) {
                    auto pos = upperText.find("TOTAL CREDITS EARNED:");
                    std::string rest = upperText.substr(pos + 21);
                    std::string creds = TextNormalizer::trim(rest.substr(0, rest.find_first_of(" C")));
                    if (!creds.empty()) {
                        summarySec.fields["total_credits_earned"] = creds;
                    }
                }
            } else if (block.type == BlockType::Heading) {
                if (TextNormalizer::startsWith(upperText, "SPRING 20") ||
                    TextNormalizer::startsWith(upperText, "FALL 20") ||
                    TextNormalizer::startsWith(upperText, "WINTER 20") ||
                    TextNormalizer::startsWith(upperText, "SUMMER 20")) {
                    courseSec.blocks.push_back(block);
                    courseSec.confidenceSummary.merge(block.confidence);
                } else {
                    otherSec.blocks.push_back(block);
                    otherSec.confidenceSummary.merge(block.confidence);
                }
            } else {
                otherSec.blocks.push_back(block);
                otherSec.confidenceSummary.merge(block.confidence);
            }
        }
    }

    // Attach tables to Course History
    if (!tables.empty()) {
        for (const auto& tbl : tables) {
            courseSec.tables.push_back(tbl);
            courseSec.confidenceSummary.merge(tbl.confidence);
        }
    }

    // Collect non-empty sections
    if (!instSec.empty()) sections.push_back(std::move(instSec));
    if (!studentSec.empty()) sections.push_back(std::move(studentSec));
    if (!academicSec.empty()) sections.push_back(std::move(academicSec));
    if (!courseSec.empty()) sections.push_back(std::move(courseSec));
    if (!summarySec.empty()) sections.push_back(std::move(summarySec));
    if (!otherSec.empty()) sections.push_back(std::move(otherSec));

    return sections;
}

void SectionDetector::extractGlobalEntities(
    const std::vector<SemanticSection>& sections,
    std::map<std::string, std::string>& outIdentity,
    std::map<std::string, std::string>& outInstitution,
    std::map<std::string, std::string>& outAcademic
) {
    for (const auto& sec : sections) {
        if (sec.type == SectionType::StudentInformation) {
            for (const auto& [k, v] : sec.fields) {
                outIdentity[k] = v;
            }
        } else if (sec.type == SectionType::InstitutionInformation) {
            for (const auto& [k, v] : sec.fields) {
                outInstitution[k] = v;
            }
        } else if (sec.type == SectionType::AcademicInformation || sec.type == SectionType::AcademicSummary) {
            for (const auto& [k, v] : sec.fields) {
                outAcademic[k] = v;
            }
        }
    }
}

} // namespace edmo::ocr::representation
