#include <ocr/OcrEngine.hpp>
#include <ocr/ErrorCode.hpp>
#include "../src/output/JsonSerializer.hpp"
#include "../src/utils/Timer.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

int main(int argc, char* argv[]) {
    std::string testPath = (argc > 1) ? argv[1] : "tests/fixtures/transcript-like.pdf";
    int iterations = (argc > 2) ? std::max(1, std::atoi(argv[2])) : 5;

    if (!std::filesystem::exists(testPath)) {
        std::cerr << "Benchmark fixture not found: " << testPath << "\n";
        return 1;
    }

    std::cout << "============================================================\n"
              << " OCR-DI Engine v0.1 — Performance Benchmark\n"
              << "============================================================\n"
              << " Target Document : " << testPath << "\n"
              << " Iterations      : " << iterations << "\n"
              << " Backend         : Tesseract OCR (CPU)\n"
              << "============================================================\n";

    ocr::OcrConfig config = ocr::OcrConfig::getProfile("transcript");
    ocr::OcrEngine engine(config);

    std::vector<double> renderTimes;
    std::vector<double> prepTimes;
    std::vector<double> ocrTimes;
    std::vector<double> serializeTimes;
    std::vector<double> totalTimes;
    size_t totalPagesProcessed = 0;

    // Warm-up run
    std::cout << "Warming up engine... " << std::flush;
    auto warmupRes = engine.process(testPath);
    if (!warmupRes) {
        std::cerr << "Warm-up failed: " << warmupRes.error().toString() << "\n";
        return 1;
    }
    std::cout << "Done.\n\nRunning benchmark iterations...\n";

    for (int i = 1; i <= iterations; ++i) {
        ocr::Timer iterTimer;
        auto result = engine.process(testPath);
        if (!result) {
            std::cerr << "Iteration " << i << " failed: " << result.error().toString() << "\n";
            return 1;
        }

        const auto& r = result.value();
        totalPagesProcessed += r.pages.size();

        ocr::Timer serTimer;
        std::string jsonStr = ocr::JsonSerializer::serialize(r, ocr::OutputFormat::FullJson, false);
        double serTime = serTimer.elapsedMs();

        renderTimes.push_back(r.timing.pdfRenderTimeMs);
        prepTimes.push_back(r.timing.preprocessTimeMs);
        ocrTimes.push_back(r.timing.ocrTimeMs);
        serializeTimes.push_back(serTime);
        totalTimes.push_back(r.timing.totalTimeMs + serTime);

        std::cout << "  [Run " << std::setw(2) << i << "/" << iterations << "] "
                  << "Pages: " << r.pages.size() << " | "
                  << "Total: " << std::fixed << std::setprecision(1) << totalTimes.back() << " ms | "
                  << "OCR: " << r.timing.ocrTimeMs << " ms | "
                  << "Conf: " << (r.confidence * 100.0f) << "%\n";
    }

    auto calcMean = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };

    double avgRender = calcMean(renderTimes);
    double avgPrep = calcMean(prepTimes);
    double avgOcr = calcMean(ocrTimes);
    double avgSer = calcMean(serializeTimes);
    double avgTotal = calcMean(totalTimes);

    double pagesPerRun = static_cast<double>(totalPagesProcessed) / iterations;
    double avgMsPerPage = avgTotal / (pagesPerRun > 0 ? pagesPerRun : 1.0);
    double pagesPerSec = (avgMsPerPage > 0) ? (1000.0 / avgMsPerPage) : 0.0;
    double docsPerSec = (avgTotal > 0) ? (1000.0 / avgTotal) : 0.0;

    std::cout << "\n============================================================\n"
              << " Benchmark Results Summary (Averages across " << iterations << " runs)\n"
              << "============================================================\n"
              << std::fixed << std::setprecision(2)
              << " PDF Rendering Latency  : " << std::setw(8) << avgRender << " ms\n"
              << " Preprocessing Latency  : " << std::setw(8) << avgPrep << " ms\n"
              << " OCR Inference Latency  : " << std::setw(8) << avgOcr << " ms\n"
              << " JSON Serialization     : " << std::setw(8) << avgSer << " ms\n"
              << "------------------------------------------------------------\n"
              << " Total End-to-End Latency: " << std::setw(8) << avgTotal << " ms\n"
              << " Average Latency / Page  : " << std::setw(8) << avgMsPerPage << " ms\n"
              << " Throughput (Pages/Sec)  : " << std::setw(8) << pagesPerSec << " pages/s\n"
              << " Throughput (Docs/Sec)   : " << std::setw(8) << docsPerSec << " docs/s\n"
              << "============================================================\n";

    return 0;
}
