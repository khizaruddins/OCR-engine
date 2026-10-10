#include "TableReconstructor.hpp"
#include "TableDetector.hpp"
#include "TextNormalizer.hpp"

#include <algorithm>

namespace ocr::representation {

SemanticRow TableReconstructor::parseCourseRow(
    const OcrLine& line,
    int pageNumber,
    const TableConfig& config
) {
    SemanticRow row;
    row.source.pageNumber = pageNumber;
    row.source.bbox = line.bbox;
    row.confidence = line.confidence;

    if (line.tokens.empty()) return row;

    const auto& tokens = line.tokens;

    // 1. Identify Course Code (check for optional serial number prefix like "1", "2", etc.)
    size_t titleStartIdx = 1;
    std::string courseCode;
    BoundingBox codeBox;
    float codeConf = 0.0f;

    bool isSerialToken = (tokens[0].text.size() <= 3 && std::isdigit(static_cast<unsigned char>(tokens[0].text[0])));
    if (isSerialToken && tokens.size() >= 2 && TableDetector::isCourseCode(tokens[1].text)) {
        courseCode = tokens[1].text;
        codeBox = tokens[1].bbox;
        codeConf = tokens[1].confidence;
        titleStartIdx = 2;
        row.canonicalFields["s_no"] = tokens[0].text;
    } else if (TableDetector::isCourseCode(tokens[0].text)) {
        courseCode = tokens[0].text;
        codeBox = tokens[0].bbox;
        codeConf = tokens[0].confidence;
        titleStartIdx = 1;
    } else if (tokens.size() >= 2 && TableDetector::isCourseCode(tokens[0].text + " " + tokens[1].text)) {
        courseCode = tokens[0].text + " " + tokens[1].text;
        codeBox = tokens[0].bbox.unionWith(tokens[1].bbox);
        codeConf = (tokens[0].confidence + tokens[1].confidence) / 2.0f;
        titleStartIdx = 2;
    } else if (tokens.size() >= 2 && TableDetector::isCourseCode(tokens[0].text + tokens[1].text)) {
        courseCode = tokens[0].text + tokens[1].text;
        codeBox = tokens[0].bbox.unionWith(tokens[1].bbox);
        codeConf = (tokens[0].confidence + tokens[1].confidence) / 2.0f;
        titleStartIdx = 2;
    } else {
        // Fallback: use first token
        courseCode = tokens[0].text;
        codeBox = tokens[0].bbox;
        codeConf = tokens[0].confidence;
        titleStartIdx = 1;
    }

    // 2. Scan from the right end for Grade and Credits
    std::string gradeVal;
    BoundingBox gradeBox;
    float gradeConf = 0.0f;

    int rightScan = static_cast<int>(tokens.size()) - 1;
    if (rightScan >= static_cast<int>(titleStartIdx) &&
        TableDetector::isGradeToken(tokens[rightScan].text, config.gradeVocabulary)) {
        gradeVal = TextNormalizer::trim(tokens[rightScan].text);
        if (TextNormalizer::toUpper(gradeVal) == "WW") {
            gradeVal = "W";
        }
        gradeBox = tokens[rightScan].bbox;
        gradeConf = tokens[rightScan].confidence;
        rightScan--;
    }

    // 3. Scan for credits before grade
    std::vector<int> creditIndices;
    while (rightScan >= static_cast<int>(titleStartIdx) &&
           creditIndices.size() < 2 &&
           TableDetector::isNumericCreditToken(tokens[rightScan].text)) {
        creditIndices.push_back(rightScan);
        rightScan--;
    }
    std::reverse(creditIndices.begin(), creditIndices.end());

    std::string creditsVal;
    BoundingBox creditsBox;
    float creditsConf = 0.0f;

    if (creditIndices.size() == 1) {
        int cIdx = creditIndices[0];
        std::string rawCr = tokens[cIdx].text;
        if (rawCr.size() >= 2 && (rawCr[0] == 'o' || rawCr[0] == 'O') && std::isdigit(static_cast<unsigned char>(rawCr[1]))) {
            rawCr[0] = '0';
        }
        creditsVal = rawCr;
        creditsBox = tokens[cIdx].bbox;
        creditsConf = tokens[cIdx].confidence;
    } else if (creditIndices.size() == 2) {
        // Two credit numbers, e.g. "03" (attempted), "03" (earned)
        // Set earned credits as primary creditsVal
        int earnIdx = creditIndices[1];
        int attIdx = creditIndices[0];
        std::string rawEarn = tokens[earnIdx].text;
        if (rawEarn.size() >= 2 && (rawEarn[0] == 'o' || rawEarn[0] == 'O') && std::isdigit(static_cast<unsigned char>(rawEarn[1]))) {
            rawEarn[0] = '0';
        }
        std::string rawAtt = tokens[attIdx].text;
        if (rawAtt.size() >= 2 && (rawAtt[0] == 'o' || rawAtt[0] == 'O') && std::isdigit(static_cast<unsigned char>(rawAtt[1]))) {
            rawAtt[0] = '0';
        }

        creditsVal = rawEarn;
        creditsBox = tokens[attIdx].bbox.unionWith(tokens[earnIdx].bbox);
        creditsConf = (tokens[attIdx].confidence + tokens[earnIdx].confidence) / 2.0f;

        row.canonicalFields["attempted_credits"] = rawAtt;
        row.canonicalFields["earned_credits"] = rawEarn;
    }

    // 4. Middle tokens form Course Title
    int titleEndIdx = rightScan;
    std::string courseTitle;
    BoundingBox titleBox;
    float sumTitleConf = 0.0f;
    int titleTokCount = 0;

    for (int t = static_cast<int>(titleStartIdx); t <= titleEndIdx; ++t) {
        if (titleTokCount > 0) {
            courseTitle.push_back(' ');
            titleBox = titleBox.unionWith(tokens[t].bbox);
        } else {
            titleBox = tokens[t].bbox;
        }
        courseTitle.append(tokens[t].text);
        sumTitleConf += tokens[t].confidence;
        titleTokCount++;
    }

    float titleConf = titleTokCount > 0 ? (sumTitleConf / static_cast<float>(titleTokCount)) : 0.0f;

    // Populate Canonical Fields
    row.canonicalFields["course_code"] = courseCode;
    row.canonicalFields["course_title"] = courseTitle;
    if (!creditsVal.empty()) {
        row.canonicalFields["credits"] = creditsVal;
    }
    if (!gradeVal.empty()) {
        row.canonicalFields["grade"] = gradeVal;
    }

    // Populate Cells
    SemanticCell cellCode;
    cellCode.rawText = courseCode;
    cellCode.normalizedText = courseCode;
    cellCode.confidence = codeConf;
    cellCode.source.pageNumber = pageNumber;
    cellCode.source.bbox = codeBox;
    row.cells.push_back(std::move(cellCode));

    SemanticCell cellTitle;
    cellTitle.rawText = courseTitle;
    cellTitle.normalizedText = courseTitle;
    cellTitle.confidence = titleConf;
    cellTitle.source.pageNumber = pageNumber;
    cellTitle.source.bbox = titleBox;
    row.cells.push_back(std::move(cellTitle));

    if (!creditsVal.empty()) {
        SemanticCell cellCr;
        cellCr.rawText = creditsVal;
        cellCr.normalizedText = creditsVal;
        cellCr.confidence = creditsConf;
        cellCr.source.pageNumber = pageNumber;
        cellCr.source.bbox = creditsBox;
        row.cells.push_back(std::move(cellCr));
    }

    if (!gradeVal.empty()) {
        SemanticCell cellGr;
        cellGr.rawText = gradeVal;
        cellGr.normalizedText = gradeVal;
        cellGr.confidence = gradeConf;
        cellGr.source.pageNumber = pageNumber;
        cellGr.source.bbox = gradeBox;
        row.cells.push_back(std::move(cellGr));
    }

    return row;
}

std::vector<SemanticColumn> TableReconstructor::buildCanonicalColumns(
    const std::vector<SemanticRow>& rows,
    const OcrLine* headerLine
) {
    (void)headerLine;
    std::vector<SemanticColumn> cols;

    bool hasAttempted = false;
    bool hasEarned = false;
    bool hasCredits = false;
    bool hasGrade = false;

    for (const auto& r : rows) {
        if (r.canonicalFields.count("attempted_credits")) hasAttempted = true;
        if (r.canonicalFields.count("earned_credits")) hasEarned = true;
        if (r.canonicalFields.count("credits")) hasCredits = true;
        if (r.canonicalFields.count("grade")) hasGrade = true;
    }

    int colIdx = 0;
    SemanticColumn colCode;
    colCode.name = "course_code";
    colCode.canonicalName = "course_code";
    colCode.columnIndex = colIdx++;
    cols.push_back(std::move(colCode));

    SemanticColumn colTitle;
    colTitle.name = "course_title";
    colTitle.canonicalName = "course_title";
    colTitle.columnIndex = colIdx++;
    cols.push_back(std::move(colTitle));

    if (hasAttempted && hasEarned) {
        SemanticColumn colAtt;
        colAtt.name = "attempted_credits";
        colAtt.canonicalName = "attempted_credits";
        colAtt.columnIndex = colIdx++;
        cols.push_back(std::move(colAtt));

        SemanticColumn colEarn;
        colEarn.name = "earned_credits";
        colEarn.canonicalName = "earned_credits";
        colEarn.columnIndex = colIdx++;
        cols.push_back(std::move(colEarn));
    } else if (hasCredits) {
        SemanticColumn colCr;
        colCr.name = "credits";
        colCr.canonicalName = "credits";
        colCr.columnIndex = colIdx++;
        cols.push_back(std::move(colCr));
    }

    if (hasGrade) {
        SemanticColumn colGr;
        colGr.name = "grade";
        colGr.canonicalName = "grade";
        colGr.columnIndex = colIdx++;
        cols.push_back(std::move(colGr));
    }

    return cols;
}

SemanticTable TableReconstructor::reconstructTable(
    const std::vector<OcrLine>& lines,
    const TableRegionCandidate& candidate,
    int pageNumber,
    int tableIdIndex,
    const TableConfig& config
) {
    SemanticTable table;
    table.id = "table_p" + std::to_string(pageNumber) + "_" + std::to_string(tableIdIndex);
    table.pageStart = pageNumber;
    table.pageEnd = pageNumber;
    table.tableType = candidate.tableType;
    table.hasHeader = candidate.hasHeader;

    const OcrLine* headerLinePtr = nullptr;
    if (candidate.hasHeader && candidate.headerLineIndex >= 0 &&
        candidate.headerLineIndex < static_cast<int>(lines.size())) {
        headerLinePtr = &lines[candidate.headerLineIndex];
    }

    BoundingBox unionBox;
    bool firstBox = true;

    for (int i = candidate.startLineIndex; i <= candidate.endLineIndex; ++i) {
        if (i < 0 || i >= static_cast<int>(lines.size())) continue;
        if (i == candidate.headerLineIndex) continue; // Skip header line from data rows

        const auto& line = lines[i];
        if (firstBox) {
            unionBox = line.bbox;
            firstBox = false;
        } else {
            unionBox = unionBox.unionWith(line.bbox);
        }

        SemanticRow row = parseCourseRow(line, pageNumber, config);
        if (!row.empty()) {
            for (const auto& cell : row.cells) {
                table.confidence.addScore(cell.confidence);
            }
            table.rows.push_back(std::move(row));
        }
    }

    table.source.pageNumber = pageNumber;
    table.source.bbox = unionBox;
    table.columns = buildCanonicalColumns(table.rows, headerLinePtr);

    return table;
}

} // namespace ocr::representation
