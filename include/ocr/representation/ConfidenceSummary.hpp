#pragma once

#include <algorithm>
#include <nlohmann/json.hpp>

namespace edmo::ocr::representation {

/**
 * @brief Statistical summary of OCR confidence metrics at a given structural hierarchy level.
 */
struct ConfidenceSummary {
    float mean = 0.0f;
    float minimum = 1.0f;
    float maximum = 0.0f;
    int tokenCount = 0;
    int lowConfidenceTokenCount = 0; // Number of tokens below low-confidence threshold

    void addScore(float score, float lowThreshold = 0.70f) noexcept {
        if (score < 0.0f) score = 0.0f;
        if (score > 1.0f) score = 1.0f;

        if (tokenCount == 0) {
            mean = score;
            minimum = score;
            maximum = score;
            tokenCount = 1;
            lowConfidenceTokenCount = (score < lowThreshold) ? 1 : 0;
        } else {
            float total = (mean * static_cast<float>(tokenCount)) + score;
            tokenCount++;
            mean = total / static_cast<float>(tokenCount);
            minimum = std::min(minimum, score);
            maximum = std::max(maximum, score);
            if (score < lowThreshold) {
                lowConfidenceTokenCount++;
            }
        }
    }

    void merge(const ConfidenceSummary& other) noexcept {
        if (other.tokenCount == 0) return;
        if (tokenCount == 0) {
            *this = other;
            return;
        }

        float totalSelf = mean * static_cast<float>(tokenCount);
        float totalOther = other.mean * static_cast<float>(other.tokenCount);
        int combinedCount = tokenCount + other.tokenCount;

        mean = (totalSelf + totalOther) / static_cast<float>(combinedCount);
        minimum = std::min(minimum, other.minimum);
        maximum = std::max(maximum, other.maximum);
        tokenCount = combinedCount;
        lowConfidenceTokenCount += other.lowConfidenceTokenCount;
    }

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"mean", tokenCount > 0 ? mean : 0.0f},
            {"minimum", tokenCount > 0 ? minimum : 0.0f},
            {"maximum", tokenCount > 0 ? maximum : 0.0f},
            {"token_count", tokenCount},
            {"low_confidence_token_count", lowConfidenceTokenCount}
        };
    }

    static ConfidenceSummary fromJson(const nlohmann::json& j) {
        ConfidenceSummary cs;
        cs.mean = j.value("mean", 0.0f);
        cs.minimum = j.value("minimum", 0.0f);
        cs.maximum = j.value("maximum", 0.0f);
        cs.tokenCount = j.value("token_count", 0);
        cs.lowConfidenceTokenCount = j.value("low_confidence_token_count", 0);
        return cs;
    }
};

} // namespace edmo::ocr::representation
