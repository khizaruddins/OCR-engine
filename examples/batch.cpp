#include <ocr/OcrEngine.hpp>
#include "../src/output/JsonSerializer.hpp"
#include "../src/utils/FileUtils.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    std::filesystem::path inputDir = (argc > 1) ? argv[1] : "documents";
    std::filesystem::path outputDir = (argc > 2) ? argv[2] : "results";

    std::cout << "Running OCR Batch Example\n"
              << "Input Dir : " << inputDir.string() << "\n"
              << "Output Dir: " << outputDir.string() << "\n\n";

    ocr::OcrConfig config = ocr::OcrConfig::getProfile("default");
    ocr::OcrEngine engine(config);

    auto files = ocr::FileUtils::listSupportedFiles(inputDir, true);
    if (files.empty()) {
        std::cout << "No supported documents found.\n";
        return 0;
    }

    std::filesystem::create_directories(outputDir);

    size_t success = 0;
    for (const auto& file : files) {
        std::cout << "Processing: " << file.filename().string() << "... ";
        auto res = engine.process(file);
        if (res) {
            success++;
            auto outPath = outputDir / (file.stem().string() + ".json");
            ocr::JsonSerializer::saveToFile(outPath, res.value(), ocr::OutputFormat::FullJson, true);
            std::cout << "OK (" << res.value().timing.totalTimeMs << " ms)\n";
        } else {
            std::cout << "FAILED (" << res.error().message << ")\n";
        }
    }

    std::cout << "\nBatch finished: " << success << "/" << files.size() << " successful.\n";
    return 0;
}
