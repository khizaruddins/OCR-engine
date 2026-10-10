#pragma once

#include <edmo/ocr/representation/RepresentationConfig.hpp>

#include <string>
#include <string_view>

namespace edmo::ocr::representation {

class TextNormalizer {
public:
    static std::string normalize(
        const std::string& input,
        const NormalizationConfig& config = {}
    );

    static std::string trim(std::string_view str);

    static std::string collapseWhitespace(std::string_view str);

    static std::string toUpper(std::string_view str);

    static std::string toLower(std::string_view str);

    static bool iequals(std::string_view a, std::string_view b);

    static bool icontains(std::string_view haystack, std::string_view needle);

    static bool startsWith(std::string_view str, std::string_view prefix);

    static bool endsWith(std::string_view str, std::string_view suffix);

    static std::string extractAlphanumeric(std::string_view str);
};

} // namespace edmo::ocr::representation
