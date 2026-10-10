#include "BlockGrouper.hpp"
#include "TextNormalizer.hpp"

#include <algorithm>
#include <cctype>

namespace edmo::ocr::representation {

bool BlockGrouper::parseLabelValue(
    const std::string& lineText,
    std::string& outLabel,
    std::string& outValue
) {
    std::string text = TextNormalizer::trim(lineText);
    if (text.empty()) return false;

    // Search for delimiters: ':', '>', or '='
    // We prefer ':' or '>' as primary key-value separators
    size_t delimPos = std::string::npos;

    for (size_t i = 1; i < text.size() - 1; ++i) {
        char c = text[i];
        if (c == ':' || c == '>') {
            // Check if left side looks like a label (mostly uppercase/letters, <= 40 chars)
            std::string left = TextNormalizer::trim(text.substr(0, i));
            if (!left.empty() && left.size() <= 40) {
                // Ensure left contains letters
                bool hasLetters = false;
                for (char lc : left) {
                    if (std::isalpha(static_cast<unsigned char>(lc))) {
                        hasLetters = true;
                        break;
                    }
                }
                if (hasLetters) {
                    delimPos = i;
                    break;
                }
            }
        }
    }

    if (delimPos != std::string::npos) {
        outLabel = TextNormalizer::trim(text.substr(0, delimPos));
        outValue = TextNormalizer::trim(text.substr(delimPos + 1));
        return !outLabel.empty();
    }

    return false;
}

bool BlockGrouper::isHeading(const std::string& lineText) {
    std::string text = TextNormalizer::trim(lineText);
    if (text.empty() || text.size() > 80) return false;

    // Check known heading keywords
    std::string upper = TextNormalizer::toUpper(text);
    if (upper == "STUDENT INFORMATION" ||
        upper == "ACADEMIC RECORD" ||
        upper == "ACADEMIC TRANSCRIPT" ||
        upper == "OFFICIAL TRANSCRIPT" ||
        upper == "COURSE HISTORY" ||
        upper == "DEGREES AWARDED" ||
        upper == "PROGRAM OF STUDY" ||
        upper == "GRADING SYSTEM" ||
        upper == "TRANSCRIPT TOTALS" ||
        upper == "END OF RECORD") {
        return true;
    }

    // Check term headers e.g. "SPRING 2024 ...", "FALL 2023 ...", "WINTER 2024 ...", "SUMMER 2024 ..."
    if (TextNormalizer::startsWith(upper, "SPRING 20") ||
        TextNormalizer::startsWith(upper, "FALL 20") ||
        TextNormalizer::startsWith(upper, "SUMMER 20") ||
        TextNormalizer::startsWith(upper, "WINTER 20") ||
        TextNormalizer::startsWith(upper, "TERM ") ||
        TextNormalizer::startsWith(upper, "SEMESTER ")) {
        return true;
    }

    // Check if line is all uppercase, short, with no trailing period
    if (text.size() <= 50 && !text.empty() && text.back() != '.') {
        bool allUpper = true;
        int letterCount = 0;
        for (char c : text) {
            if (std::isalpha(static_cast<unsigned char>(c))) {
                letterCount++;
                if (!std::isupper(static_cast<unsigned char>(c))) {
                    allUpper = false;
                    break;
                }
            }
        }
        if (allUpper && letterCount >= 4) {
            return true;
        }
    }

    return false;
}

BlockType BlockGrouper::classifyLine(
    const OcrLine& line,
    int pageHeight,
    std::string& outLabel,
    std::string& outValue
) {
    std::string norm = TextNormalizer::trim(line.text);
    if (norm.empty()) return BlockType::Unknown;

    // 1. Check Header / Footer by Y position
    if (pageHeight > 0) {
        if (line.bbox.bottom() <= static_cast<int>(pageHeight * 0.08)) {
            return BlockType::Header;
        }
        if (line.bbox.y >= static_cast<int>(pageHeight * 0.92)) {
            return BlockType::Footer;
        }
    }

    // 2. Check Label-Value
    if (parseLabelValue(line.text, outLabel, outValue)) {
        return BlockType::LabelValue;
    }

    // 3. Check Heading
    if (isHeading(line.text)) {
        return BlockType::Heading;
    }

    return BlockType::Paragraph;
}

std::vector<RepresentationBlock> BlockGrouper::groupLines(
    const std::vector<OcrLine>& lines,
    int pageNumber,
    int pageWidth,
    int pageHeight,
    const RepresentationConfig& config
) {
    (void)pageWidth;
    if (lines.empty()) return {};

    std::vector<RepresentationBlock> blocks;
    int blockIdx = 1;

    for (const auto& line : lines) {
        std::string label, value;
        BlockType type = classifyLine(line, pageHeight, label, value);

        RepresentationBlock block;
        block.id = "block_p" + std::to_string(pageNumber) + "_" + std::to_string(blockIdx++);
        block.type = type;
        block.text = line.text;
        block.normalizedText = TextNormalizer::normalize(line.text, config.normalization);
        block.label = label;
        block.value = value;
        block.bbox = line.bbox;
        block.lines = {line};

        // Source reference
        block.source.pageNumber = pageNumber;
        block.source.bbox = line.bbox;

        // Confidence
        for (const auto& tok : line.tokens) {
            block.confidence.addScore(tok.confidence, static_cast<float>(config.compression.lowConfidenceThreshold));
        }
        if (line.tokens.empty()) {
            block.confidence.addScore(line.confidence, static_cast<float>(config.compression.lowConfidenceThreshold));
        }

        blocks.push_back(std::move(block));
    }

    return blocks;
}

} // namespace edmo::ocr::representation
