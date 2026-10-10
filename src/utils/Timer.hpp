#pragma once

#include <chrono>

namespace edmo::ocr {

/**
 * @brief High-precision timer utility for latency tracking across pipeline stages.
 */
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    [[nodiscard]] double elapsedMs() const {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> diff = now - start_;
        return diff.count();
    }

    [[nodiscard]] double elapsedSeconds() const {
        return elapsedMs() / 1000.0;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

} // namespace edmo::ocr
