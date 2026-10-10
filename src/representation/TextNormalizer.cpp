#include "TextNormalizer.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ocr::representation {

std::string TextNormalizer::trim(std::string_view str) {
    size_t first = 0;
    while (first < str.size() && (std::isspace(static_cast<unsigned char>(str[first])) || str[first] == '\0')) {
        ++first;
    }
    if (first == str.size()) {
        return "";
    }
    size_t last = str.size() - 1;
    while (last > first && (std::isspace(static_cast<unsigned char>(str[last])) || str[last] == '\0')) {
        --last;
    }
    return std::string(str.substr(first, last - first + 1));
}

std::string TextNormalizer::collapseWhitespace(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    bool inSpace = false;
    for (char ch : str) {
        if (ch == ' ' || ch == '\t' || ch == '\f' || ch == '\v') {
            if (!inSpace) {
                result.push_back(' ');
                inSpace = true;
            }
        } else {
            result.push_back(ch);
            inSpace = false;
        }
    }
    return trim(result);
}

std::string TextNormalizer::toUpper(std::string_view str) {
    std::string res;
    res.reserve(str.size());
    for (char c : str) {
        res.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return res;
}

std::string TextNormalizer::toLower(std::string_view str) {
    std::string res;
    res.reserve(str.size());
    for (char c : str) {
        res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return res;
}

bool TextNormalizer::iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool TextNormalizer::icontains(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) return true;
    if (haystack.size() < needle.size()) return false;

    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) {
            return std::tolower(static_cast<unsigned char>(ch1)) ==
                   std::tolower(static_cast<unsigned char>(ch2));
        }
    );
    return it != haystack.end();
}

bool TextNormalizer::startsWith(std::string_view str, std::string_view prefix) {
    if (str.size() < prefix.size()) return false;
    return str.substr(0, prefix.size()) == prefix;
}

bool TextNormalizer::endsWith(std::string_view str, std::string_view suffix) {
    if (str.size() < suffix.size()) return false;
    return str.substr(str.size() - suffix.size()) == suffix;
}

std::string TextNormalizer::extractAlphanumeric(std::string_view str) {
    std::string res;
    res.reserve(str.size());
    for (char c : str) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            res.push_back(c);
        }
    }
    return res;
}

std::string TextNormalizer::normalize(
    const std::string& input,
    const NormalizationConfig& config
) {
    if (!config.enabled || input.empty()) {
        return input;
    }

    std::string processed;
    processed.reserve(input.size());

    // 1. Normalize line breaks and remove null characters
    for (size_t i = 0; i < input.size(); ++i) {
        char ch = input[i];
        if (ch == '\r') {
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                continue; // skip \r before \n
            }
            if (config.normalizeLineBreaks) {
                processed.push_back('\n');
            } else {
                processed.push_back(ch);
            }
        } else if (ch != '\0') {
            processed.push_back(ch);
        }
    }

    // 2. Collapse line-by-line whitespace if requested
    if (config.collapseWhitespace || config.trimWhitespace) {
        std::istringstream stream(processed);
        std::string line;
        std::string result;
        bool firstLine = true;

        while (std::getline(stream, line)) {
            std::string cleanLine = line;
            if (config.collapseWhitespace) {
                cleanLine = collapseWhitespace(cleanLine);
            } else if (config.trimWhitespace) {
                cleanLine = trim(cleanLine);
            }

            if (!firstLine) {
                result.push_back('\n');
            }
            result.append(cleanLine);
            firstLine = false;
        }

        if (config.trimWhitespace) {
            return trim(result);
        }
        return result;
    }

    if (config.trimWhitespace) {
        return trim(processed);
    }

    return processed;
}

} // namespace ocr::representation
