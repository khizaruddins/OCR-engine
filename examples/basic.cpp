#include <edmo/ocr/OcrEngine.hpp>
#include "../src/output/JsonSerializer.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    std::string inputPath = (argc > 1) ? argv[1] : "sample.png";

    std::cout << "Running EDMO OCR Engine basic example on: " << inputPath << "\n";

    edmo::ocr::OcrConfig config = edmo::ocr::OcrConfig::getProfile("transcript");
    edmo::ocr::OcrEngine engine(config);

    auto result = engine.process(inputPath);
    if (!result) {
        std::cerr << "Processing failed: " << result.error().toString() << "\n";
        return 1;
    }

    std::cout << "Processed in " << result.value().timing.totalTimeMs << " ms\n"
              << "Average confidence: " << (result.value().confidence * 100.0f) << "%\n\n";

    std::cout << "JSON Output:\n"
              << edmo::ocr::JsonSerializer::serialize(result.value(), edmo::ocr::OutputFormat::FullJson, true)
              << "\n";

    return 0;
}
