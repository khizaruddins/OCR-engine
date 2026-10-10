#include "NoiseReducer.hpp"
#include "TextNormalizer.hpp"

#include <cctype>

namespace edmo::ocr::representation {

bool NoiseReducer::isNoiseToken(
    const OcrToken& token,
    const NoiseReductionConfig& config
) {
    if (!config.enabled) return false;

    std::string text = TextNormalizer::trim(token.text);
    if (text.empty()) return true;

    if (config.removeIsolatedSymbols) {
        // Isolated single-character noise (pipe, tilde, backtick, caret, underscore)
        if (text.size() == 1) {
            char c = text[0];
            if (c == '|' || c == '~' || c == '^' || c == '`' || c == '\\' || c == '_') {
                return true;
            }
        }
        // Repetitive symbols like "---", "___", "...", "|||"
        if (text.size() >= 2) {
            bool allSameSymbol = true;
            char first = text[0];
            if (!std::isalnum(static_cast<unsigned char>(first))) {
                for (char c : text) {
                    if (c != first) {
                        allSameSymbol = false;
                        break;
                    }
                }
                if (allSameSymbol && (first == '_' || first == '|' || first == '~' || first == '^')) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool NoiseReducer::isNoiseLine(
    const OcrLine& line,
    const NoiseReductionConfig& config
) {
    if (!config.enabled) return false;

    std::string trimmed = TextNormalizer::trim(line.text);
    if (trimmed.empty()) return true;

    // Check if line has any alphanumeric content
    bool hasAlphanumeric = false;
    for (char c : trimmed) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            hasAlphanumeric = true;
            break;
        }
    }

    if (!hasAlphanumeric && config.removeIsolatedSymbols) {
        // If line is only stray symbols e.g. " | " or " _ " or " --- "
        if (trimmed == "|" || trimmed == "_" || trimmed == "~" || trimmed == "^" || trimmed == "\\") {
            return true;
        }
    }

    return false;
}

bool NoiseReducer::isNoiseBlock(
    const OcrBlock& block,
    const NoiseReductionConfig& config
) {
    if (!config.enabled) return false;

    if (config.removeEmptyBlocks) {
        bool allEmpty = true;
        for (const auto& line : block.lines) {
            if (!isNoiseLine(line, config)) {
                allEmpty = false;
                break;
            }
        }
        if (allEmpty && TextNormalizer::trim(block.text).empty()) {
            return true;
        }
    }

    return false;
}

std::vector<OcrLine> NoiseReducer::filterLines(
    const std::vector<OcrLine>& lines,
    const NoiseReductionConfig& config
) {
    if (!config.enabled) return lines;

    std::vector<OcrLine> filtered;
    filtered.reserve(lines.size());

    for (const auto& line : lines) {
        if (!isNoiseLine(line, config)) {
            OcrLine cleanLine = line;
            if (!line.tokens.empty()) {
                std::vector<OcrToken> cleanTokens;
                cleanTokens.reserve(line.tokens.size());
                for (const auto& tok : line.tokens) {
                    if (!isNoiseToken(tok, config)) {
                        cleanTokens.push_back(tok);
                    }
                }
                cleanLine.tokens = std::move(cleanTokens);
            }
            filtered.push_back(std::move(cleanLine));
        }
    }

    return filtered;
}

std::vector<OcrBlock> NoiseReducer::filterBlocks(
    const std::vector<OcrBlock>& blocks,
    const NoiseReductionConfig& config
) {
    if (!config.enabled) return blocks;

    std::vector<OcrBlock> filtered;
    filtered.reserve(blocks.size());

    for (const auto& block : blocks) {
        if (!isNoiseBlock(block, config)) {
            OcrBlock cleanBlock = block;
            cleanBlock.lines = filterLines(block.lines, config);
            if (!cleanBlock.lines.empty() || !TextNormalizer::trim(cleanBlock.text).empty()) {
                filtered.push_back(std::move(cleanBlock));
            }
        }
    }

    return filtered;
}

} // namespace edmo::ocr::representation
