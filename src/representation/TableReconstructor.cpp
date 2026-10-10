#include "TableReconstructor.hpp"
#include "TableDetector.hpp"
#include "TextNormalizer.hpp"
#include "ReadingOrder.hpp"

#include <algorithm>
#include <iostream>

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
    bool hasValidCode = false;
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
        hasValidCode = true;
    } else if (TableDetector::isCourseCode(tokens[0].text)) {
        courseCode = tokens[0].text;
        codeBox = tokens[0].bbox;
        codeConf = tokens[0].confidence;
        titleStartIdx = 1;
        hasValidCode = true;
    } else if (tokens.size() >= 2 && TableDetector::isCourseCode(tokens[0].text + " " + tokens[1].text)) {
        courseCode = tokens[0].text + " " + tokens[1].text;
        codeBox = tokens[0].bbox.unionWith(tokens[1].bbox);
        codeConf = (tokens[0].confidence + tokens[1].confidence) / 2.0f;
        titleStartIdx = 2;
        hasValidCode = true;
    } else if (tokens.size() >= 2 && TableDetector::isCourseCode(tokens[0].text + tokens[1].text)) {
        courseCode = tokens[0].text + tokens[1].text;
        codeBox = tokens[0].bbox.unionWith(tokens[1].bbox);
        codeConf = (tokens[0].confidence + tokens[1].confidence) / 2.0f;
        titleStartIdx = 2;
        hasValidCode = true;
    }

    if (!hasValidCode) {
        // Not a course row line
        return row;
    }

    // 2. Scan from the right end for Grade and Credits
    std::string gradeVal;
    BoundingBox gradeBox;
    float gradeConf = 0.0f;

    int rightScan = static_cast<int>(tokens.size()) - 1;
    if (rightScan >= static_cast<int>(titleStartIdx) &&
        TableDetector::isGradeToken(tokens[rightScan].text, config.gradeVocabulary)) {
        gradeVal = TableDetector::normalizeGrade(tokens[rightScan].text);
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
        std::string tokText = tokens[t].text;
        // Normalize OCR pipe '|' to roman numeral 'I' if it occurs in course title (e.g. Accounting | -> Accounting I)
        if (tokText == "|" || tokText == "!") {
            tokText = "I";
        }
        if (titleTokCount > 0) {
            courseTitle.push_back(' ');
            titleBox = titleBox.unionWith(tokens[t].bbox);
        } else {
            titleBox = tokens[t].bbox;
        }
        courseTitle.append(tokText);
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

    bool headerHasCredits = false;
    bool headerHasGrade = false;
    if (headerLine) {
        std::string upperHeader = TextNormalizer::toUpper(headerLine->text);
        if (TextNormalizer::icontains(upperHeader, "CREDIT") ||
            TextNormalizer::icontains(upperHeader, "CREDS") ||
            TextNormalizer::icontains(upperHeader, "HRS") ||
            TextNormalizer::icontains(upperHeader, "UNITS")) {
            headerHasCredits = true;
        }
        if (TextNormalizer::icontains(upperHeader, "GRADE") ||
            TextNormalizer::icontains(upperHeader, "GRD") ||
            TextNormalizer::icontains(upperHeader, " GR ") ||
            TextNormalizer::endsWith(upperHeader, " GR") ||
            TextNormalizer::endsWith(upperHeader, " GR.") ||
            TextNormalizer::icontains(upperHeader, "MARK")) {
            headerHasGrade = true;
        }
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
    } else if (hasCredits || headerHasCredits) {
        SemanticColumn colCr;
        colCr.name = "credits";
        colCr.canonicalName = "credits";
        colCr.columnIndex = colIdx++;
        cols.push_back(std::move(colCr));
    }

    if (hasGrade || headerHasGrade) {
        SemanticColumn colGr;
        colGr.name = "grade";
        colGr.canonicalName = "grade";
        colGr.columnIndex = colIdx++;
        cols.push_back(std::move(colGr));
    }

    // Set spatial bboxes from header line tokens if available
    if (headerLine && !headerLine->tokens.empty()) {
        for (const auto& tok : headerLine->tokens) {
            std::string upperTok = TextNormalizer::toUpper(tok.text);
            for (auto& col : cols) {
                if (col.canonicalName == "grade" && (upperTok == "GRADE" || upperTok == "GR" || upperTok == "GRD" || upperTok == "MARK" || upperTok == "MARKS")) {
                    col.minX = tok.bbox.x;
                    col.maxX = tok.bbox.right();
                    col.confidence = tok.confidence;
                    col.source.bbox = tok.bbox;
                } else if (col.canonicalName == "credits" && (upperTok == "CREDITS" || upperTok == "CREDIT" || upperTok == "CREDS" || upperTok == "HRS" || upperTok == "UNITS")) {
                    col.minX = tok.bbox.x;
                    col.maxX = tok.bbox.right();
                    col.confidence = tok.confidence;
                    col.source.bbox = tok.bbox;
                } else if (col.canonicalName == "course_title" && (upperTok == "TITLE" || upperTok == "DESCRIPTION" || upperTok == "NAME")) {
                    if (col.minX == 0 || tok.bbox.x < col.minX) col.minX = tok.bbox.x;
                    if (tok.bbox.right() > col.maxX) col.maxX = tok.bbox.right();
                    col.confidence = tok.confidence;
                    col.source.bbox = tok.bbox;
                } else if (col.canonicalName == "course_code" && (upperTok == "COURSE" || upperTok == "CRS" || upperTok == "SUBJ" || upperTok == "CODE" || upperTok == "TEACH")) {
                    if (col.minX == 0 || tok.bbox.x < col.minX) col.minX = tok.bbox.x;
                    if (tok.bbox.right() > col.maxX) col.maxX = tok.bbox.right();
                    col.confidence = tok.confidence;
                    col.source.bbox = tok.bbox;
                }
            }
        }
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

    // 1. Collect candidate lines and identify header line(s)
    const OcrLine* headerLinePtr = nullptr;
    if (candidate.hasHeader && candidate.headerLineIndex >= 0 &&
        candidate.headerLineIndex < static_cast<int>(lines.size())) {
        headerLinePtr = &lines[candidate.headerLineIndex];
    }

    std::vector<OcrLine> dataLines;
    for (int i = candidate.startLineIndex; i <= candidate.endLineIndex; ++i) {
        if (i < 0 || i >= static_cast<int>(lines.size())) continue;
        const auto& line = lines[i];

        if (i == candidate.headerLineIndex || TableDetector::isTableHeaderLine(line)) {
            if (!headerLinePtr || line.tokens.size() > headerLinePtr->tokens.size()) {
                headerLinePtr = &line;
            }
            continue; // Header lines must NOT become course rows
        }
        dataLines.push_back(line);
    }

    // 2. Group horizontally overlapping / split lines on the same physical row (Row Grouping by Vertical Overlap)
    // Grade tokens (like narrow "A" or "B") that overlap vertically with the course row MUST belong to the same physical row.
    std::vector<OcrLine> physicalLines;
    for (const auto& dLine : dataLines) {
        if (dLine.tokens.empty()) continue;

        bool merged = false;
        for (auto& pLine : physicalLines) {
            int overlapY = std::max(0, std::min(pLine.bbox.bottom(), dLine.bbox.bottom()) - std::max(pLine.bbox.top(), dLine.bbox.top()));
            int minH = std::min(std::max(1, pLine.bbox.height), std::max(1, dLine.bbox.height));
            int pCenterY = pLine.bbox.y + pLine.bbox.height / 2;
            int dCenterY = dLine.bbox.y + dLine.bbox.height / 2;
            if (overlapY >= minH / 3 || std::abs(pCenterY - dCenterY) <= minH / 2) {
                // Same physical row: merge tokens and union bbox
                pLine.bbox = pLine.bbox.unionWith(dLine.bbox);
                pLine.tokens.insert(pLine.tokens.end(), dLine.tokens.begin(), dLine.tokens.end());
                ReadingOrder::sortTokensInLine(pLine);

                std::string fullText;
                for (size_t ti = 0; ti < pLine.tokens.size(); ++ti) {
                    if (ti > 0) fullText.push_back(' ');
                    fullText.append(pLine.tokens[ti].text);
                }
                pLine.text = std::move(fullText);
                merged = true;
                break;
            }
        }
        if (!merged) {
            physicalLines.push_back(dLine);
        }
    }

    // Sort physical lines top-to-bottom
    std::sort(physicalLines.begin(), physicalLines.end(), [](const OcrLine& a, const OcrLine& b) {
        return a.bbox.y < b.bbox.y;
    });

    // 3. Parse physical rows and handle continuation lines (e.g. AUDT 301 second title line "Practice")
    BoundingBox unionBox;
    bool firstBox = true;

    for (const auto& pLine : physicalLines) {
        SemanticRow row = parseCourseRow(pLine, pageNumber, config);
        if (!row.empty()) {
            if (firstBox) {
                unionBox = pLine.bbox;
                firstBox = false;
            } else {
                unionBox = unionBox.unionWith(pLine.bbox);
            }
            for (const auto& cell : row.cells) {
                table.confidence.addScore(cell.confidence);
            }
            table.rows.push_back(std::move(row));
        } else if (!table.rows.empty()) {
            // Check if pLine is a continuation of the preceding course row's title
            auto& prevRow = table.rows.back();
            bool hasCodeOrGrades = TableDetector::isCourseRowLine(pLine, config);
            int verticalGap = pLine.bbox.y - prevRow.source.bbox.bottom();
            int maxAllowedGap = std::max(prevRow.source.bbox.height, pLine.bbox.height) * 2;

            if (!hasCodeOrGrades && verticalGap <= maxAllowedGap) {
                // Continuation line: append to preceding row's title
                prevRow.canonicalFields["course_title"] += " " + pLine.text;
                if (prevRow.cells.size() >= 2) {
                    prevRow.cells[1].rawText += " " + pLine.text;
                    prevRow.cells[1].normalizedText += " " + pLine.text;
                    prevRow.cells[1].source.bbox = prevRow.cells[1].source.bbox.unionWith(pLine.bbox);
                }
                prevRow.source.bbox = prevRow.source.bbox.unionWith(pLine.bbox);
                unionBox = unionBox.unionWith(pLine.bbox);
            }
        }
    }

    table.source.pageNumber = pageNumber;
    table.source.bbox = unionBox;
    table.columns = buildCanonicalColumns(table.rows, headerLinePtr);

    // Requirement 15: Log / print the normalized table before final projection
    std::cout << "==================================================\n";
    std::cout << "NORMALIZED TABLE: " << table.id << "\n";
    std::cout << "COLUMNS:\n";
    for (size_t ci = 0; ci < table.columns.size(); ++ci) {
        if (ci > 0) std::cout << " | ";
        std::cout << table.columns[ci].canonicalName;
    }
    std::cout << "\nROWS (count: " << table.rows.size() << "):\n";
    for (size_t ri = 0; ri < table.rows.size(); ++ri) {
        const auto& r = table.rows[ri];
        std::cout << (ri + 1) << " "
                  << (r.canonicalFields.count("course_code") ? r.canonicalFields.at("course_code") : "") << " | "
                  << (r.canonicalFields.count("course_title") ? r.canonicalFields.at("course_title") : "") << " | "
                  << (r.canonicalFields.count("credits") ? r.canonicalFields.at("credits") : "") << " | "
                  << (r.canonicalFields.count("grade") ? r.canonicalFields.at("grade") : "") << "\n";
    }
    std::cout << "==================================================" << std::endl;

    return table;
}

} // namespace ocr::representation
