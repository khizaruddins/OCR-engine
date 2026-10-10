#include "TableDetector.hpp"
#include "TextNormalizer.hpp"

#include <cctype>

namespace ocr::representation {

bool TableDetector::isCourseCode(std::string_view token) {
    std::string clean = TextNormalizer::trim(token);
    if (clean.size() < 4 || clean.size() > 10) return false;

    std::string upper = TextNormalizer::toUpper(clean);
    if (upper == "ATT" || upper == "EARN" || upper == "UNITS" || upper == "HOURS" ||
        upper == "COURSE" || upper == "SUBJ" || upper == "CRSE" || upper == "TITLE" ||
        upper == "GRADE" || upper == "SPRING" || upper == "FALL" || upper == "SUMMER" ||
        upper == "WINTER" || upper == "TERM" || upper == "CUMGPA" || upper == "GPA" ||
        upper == "TOTALS" || upper == "TOTAL" || upper == "PAGE" || upper == "CREDITS" ||
        upper == "POINTS" || upper == "RECORD") {
        return false;
    }

    // Check US/standard pattern: [A-Z]{2,5}[ -]?[0-9]{2,4}[A-Z]?
    size_t i = 0;
    size_t letters = 0;
    while (i < clean.size() && std::isalpha(static_cast<unsigned char>(clean[i]))) {
        letters++;
        i++;
    }

    if (letters >= 2 && letters <= 5) {
        if (i < clean.size() && (clean[i] == '-' || clean[i] == ' ')) {
            i++;
        }
        size_t digits = 0;
        while (i < clean.size() && std::isdigit(static_cast<unsigned char>(clean[i]))) {
            digits++;
            i++;
        }
        if (digits >= 2 && digits <= 4) {
            if (i < clean.size() && std::isalpha(static_cast<unsigned char>(clean[i]))) {
                i++;
            }
            if (i == clean.size()) return true;
        }
    }

    // Check international pattern: [0-9]{1,2}[A-Z]{2,4}[0-9]{2,4}[A-Z]? (e.g. 22BS103, 20CS301)
    i = 0;
    size_t prefixDigits = 0;
    while (i < clean.size() && std::isdigit(static_cast<unsigned char>(clean[i]))) {
        prefixDigits++;
        i++;
    }
    if (prefixDigits >= 1 && prefixDigits <= 2) {
        size_t midLetters = 0;
        while (i < clean.size() && std::isalpha(static_cast<unsigned char>(clean[i]))) {
            midLetters++;
            i++;
        }
        if (midLetters >= 2 && midLetters <= 4) {
            size_t suffixDigits = 0;
            while (i < clean.size() && std::isdigit(static_cast<unsigned char>(clean[i]))) {
                suffixDigits++;
                i++;
            }
            if (suffixDigits >= 2 && suffixDigits <= 4) {
                if (i < clean.size() && std::isalpha(static_cast<unsigned char>(clean[i]))) {
                    i++;
                }
                if (i == clean.size()) return true;
            }
        }
    }

    return false;
}

std::string TableDetector::normalizeGrade(std::string_view token) {
    std::string trimmed = TextNormalizer::trim(token);
    if (trimmed.empty()) return "";

    // 1 char: e.g. "A", "b" -> "A", "B"
    if (trimmed.size() == 1) {
        return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(trimmed[0]))));
    }

    // 2 chars:
    if (trimmed.size() == 2) {
        char c0 = static_cast<char>(std::toupper(static_cast<unsigned char>(trimmed[0])));
        char c1 = trimmed[1];
        // e.g. "A+", "B-"
        if (c1 == '+' || c1 == '-') {
            return std::string(1, c0) + c1;
        }
        // e.g. "Cc" -> "C", "Ww" -> "W", "AA" -> "A", "CC" -> "C", "WW" -> "W"
        if (std::isalpha(static_cast<unsigned char>(c1))) {
            char c1Upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c1)));
            if (std::islower(static_cast<unsigned char>(c1)) || c1Upper == c0) {
                return std::string(1, c0);
            }
        }
    }

    return TextNormalizer::toUpper(trimmed);
}

bool TableDetector::isGradeToken(
    std::string_view token,
    const std::vector<std::string>& gradeVocab
) {
    std::string norm = normalizeGrade(token);
    if (norm.empty()) return false;

    for (const auto& g : gradeVocab) {
        if (norm == g) return true;
    }

    // Additional common academic grade indicators
    if (norm == "PASS" || norm == "FAIL" || norm == "HONORS" || norm == "SATISFACTORY" ||
        norm == "P" || norm == "F" || norm == "AUDIT" || norm == "IP" || norm == "IN PROGRESS") {
        return true;
    }

    return false;
}

bool TableDetector::isNumericCreditToken(std::string_view token) {
    std::string trimmed = TextNormalizer::trim(token);
    if (trimmed.empty() || trimmed.size() > 5) return false;

    // Handle OCR confusion where 0 is recognized as 'o' or 'O', e.g. "o1", "o2", "o3"
    std::string norm = trimmed;
    if (norm.size() >= 2 && (norm[0] == 'o' || norm[0] == 'O') && std::isdigit(static_cast<unsigned char>(norm[1]))) {
        norm[0] = '0';
    }

    bool hasDigit = false;
    bool hasDot = false;

    for (char c : norm) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            hasDigit = true;
        } else if (c == '.' && !hasDot) {
            hasDot = true;
        } else {
            return false;
        }
    }

    return hasDigit;
}

bool TableDetector::isHeaderToken(std::string_view token) {
    std::string upper = TextNormalizer::toUpper(TextNormalizer::trim(token));
    if (upper == "COURSE" || upper == "CRSE" || upper == "SUBJ" || upper == "SUBJECT" ||
        upper == "TITLE" || upper == "DESCRIPTION" || upper == "NAME" ||
        upper == "CREDIT" || upper == "CREDITS" || upper == "CREDS" ||
        upper == "GRADE" || upper == "GRADES" || upper == "GR" || upper == "GRD" ||
        upper == "ATT" || upper == "EARN" || upper == "UNITS" || upper == "HOURS" ||
        upper == "QPTS" || upper == "MARKS" || upper == "TEACH" || upper == "CRS" ||
        upper == "FINAL" || upper == "CODE") {
        return true;
    }
    return false;
}

bool TableDetector::isTableHeaderLine(const OcrLine& line) {
    std::string upper = TextNormalizer::toUpper(line.text);

    int matches = 0;
    if (TextNormalizer::icontains(upper, "COURSE") || TextNormalizer::icontains(upper, "SUBJECT") || TextNormalizer::icontains(upper, "SUBJ") || TextNormalizer::icontains(upper, "CRSE") || TextNormalizer::icontains(upper, "CRS")) matches++;
    if (TextNormalizer::icontains(upper, "TITLE") || TextNormalizer::icontains(upper, "NAME") || TextNormalizer::icontains(upper, "DESCRIPTION")) matches++;
    if (TextNormalizer::icontains(upper, "CREDIT") || TextNormalizer::icontains(upper, "CREDS") || TextNormalizer::icontains(upper, "CREDITS")) matches++;
    if (TextNormalizer::icontains(upper, "GRADE") || TextNormalizer::icontains(upper, " GR ") || TextNormalizer::endsWith(upper, " GR") || TextNormalizer::endsWith(upper, " GR.") || TextNormalizer::icontains(upper, "GRD")) matches++;
    if (TextNormalizer::icontains(upper, "ATT") || TextNormalizer::icontains(upper, "EARN") || TextNormalizer::icontains(upper, "EHRS") || TextNormalizer::icontains(upper, "AHRS")) matches++;
    if (TextNormalizer::icontains(upper, "UNITS") || TextNormalizer::icontains(upper, "HOURS") || TextNormalizer::icontains(upper, "QPTS")) matches++;
    if (TextNormalizer::icontains(upper, "S.NO") || TextNormalizer::icontains(upper, "SL.NO") || TextNormalizer::icontains(upper, "STATUS") || TextNormalizer::icontains(upper, "MARKS")) matches++;

    if (matches >= 2) return true;

    // Check if line consists solely of header tokens (e.g. split header like "CREDITS GRADE" or "GRADE")
    if (!line.tokens.empty()) {
        bool allHeaderTokens = true;
        for (const auto& tok : line.tokens) {
            if (!isHeaderToken(tok.text)) {
                allHeaderTokens = false;
                break;
            }
        }
        if (allHeaderTokens) return true;
    }

    return false;
}

bool TableDetector::isCourseRowLine(
    const OcrLine& line,
    const TableConfig& config
) {
    if (line.tokens.empty()) return false;
    if (isTableHeaderLine(line)) return false;

    // Check if the line begins with or contains a course code
    bool hasCourseCode = false;
    bool hasGradeOrCredits = false;

    // Check first 2 tokens for course code
    size_t checkLimit = std::min(size_t(3), line.tokens.size());
    for (size_t i = 0; i < checkLimit; ++i) {
        if (isCourseCode(line.tokens[i].text)) {
            hasCourseCode = true;
            break;
        }
    }

    // Check if tokens 0 and 1 combined form a course code (e.g. "CS" + "101" or "MATH" + "101")
    if (!hasCourseCode && line.tokens.size() >= 2) {
        std::string combined = line.tokens[0].text + line.tokens[1].text;
        if (isCourseCode(combined)) {
            hasCourseCode = true;
        }
        std::string combinedSp = line.tokens[0].text + " " + line.tokens[1].text;
        if (isCourseCode(combinedSp)) {
            hasCourseCode = true;
        }
    }

    if (!hasCourseCode) return false;

    // Check trailing tokens for credits or grade
    for (size_t i = 1; i < line.tokens.size(); ++i) {
        const auto& text = line.tokens[i].text;
        if (isGradeToken(text, config.gradeVocabulary) || isNumericCreditToken(text)) {
            hasGradeOrCredits = true;
            break;
        }
    }

    return hasCourseCode || hasGradeOrCredits;
}

std::vector<TableRegionCandidate> TableDetector::detectTableRegions(
    const std::vector<OcrLine>& lines,
    const TableConfig& config
) {
    if (!config.enabled || lines.empty()) return {};

    std::vector<TableRegionCandidate> candidates;

    size_t i = 0;
    while (i < lines.size()) {
        const auto& line = lines[i];

        bool isHeader = isTableHeaderLine(line);
        bool isCourseRow = isCourseRowLine(line, config);

        if (isHeader || isCourseRow) {
            int startIdx = static_cast<int>(i);
            int headerIdx = isHeader ? static_cast<int>(i) : -1;
            int endIdx = static_cast<int>(i);
            int courseRowCount = isCourseRow ? 1 : 0;

            size_t j = i + 1;
            int nonTableStreak = 0;

            while (j < lines.size()) {
                const auto& nextLine = lines[j];

                if (isTableHeaderLine(nextLine)) {
                    if (headerIdx == -1) headerIdx = static_cast<int>(j);
                    endIdx = static_cast<int>(j);
                    nonTableStreak = 0;
                } else if (isCourseRowLine(nextLine, config)) {
                    endIdx = static_cast<int>(j);
                    courseRowCount++;
                    nonTableStreak = 0;
                } else {
                    // Check if it's a term summary line or GPA line right below the table, e.g. "ATT EARN UNITS" or "TERM GPA: ..."
                    std::string upper = TextNormalizer::toUpper(nextLine.text);
                    if (courseRowCount >= 1 && (
                        TextNormalizer::icontains(upper, "TERM GPA") ||
                        TextNormalizer::icontains(upper, "CUM GPA") ||
                        TextNormalizer::icontains(upper, "CUMGPA") ||
                        TextNormalizer::icontains(upper, "TERM TOTALS") ||
                        TextNormalizer::icontains(upper, "CUM TOTALS") ||
                        TextNormalizer::icontains(upper, "TOTAL CREDITS"))) {
                        // Table ended
                        break;
                    }

                    nonTableStreak++;
                    if (nonTableStreak >= 2) {
                        break;
                    }
                }
                j++;
            }

            if (courseRowCount >= 1 || (isHeader && endIdx > startIdx)) {
                TableRegionCandidate candidate;
                candidate.startLineIndex = startIdx;
                candidate.endLineIndex = endIdx;
                candidate.hasHeader = (headerIdx != -1);
                candidate.headerLineIndex = headerIdx;
                candidate.confidence = courseRowCount >= 1 ? 0.95f : 0.75f;
                candidate.tableType = "course_records";

                candidates.push_back(candidate);
                i = static_cast<size_t>(endIdx + 1);
                continue;
            }
        }

        i++;
    }

    return candidates;
}

} // namespace ocr::representation
