# OCR-DI — C++ OCR Engine v0.1

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![CMake](https://img.shields.io/badge/CMake-3.22%2B-brightgreen.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)

**OCR-DI** is an education-focused Document Intelligence platform. This repository contains the standalone, high-performance **C++ OCR Engine v0.1**, designed to ingest multi-page academic transcripts, marksheets, degree certificates, syllabi, and educational forms, returning deterministic, structured OCR JSON with full geometry and confidence metrics.

---

## Key Features

- **Multi-Format Ingestion**: Supports PNG, JPEG, TIFF, BMP, and multi-page PDF documents.
- **High-Fidelity PDF Rendering**: Built-in Poppler rendering at configurable resolutions (150, 200, 300, 400, 600 DPI).
- **Configurable Preprocessing Pipeline**:
  - **Orientation Detection & Correction**: Automatic 0°, 90°, 180°, 270° orientation detection using Tesseract OSD and rotation correction.
  - **Accurate Deskewing**: Probabilistic Hough Transform / contour line detection correcting document scan skews within `[-10.0°, +10.0°]`.
  - **Denoising**: Gaussian, median, and bilateral filtering.
  - **Contrast Enhancement**: Contrast Limited Adaptive Histogram Equalization (CLAHE) & Histogram Equalization.
  - **Binarization**: Otsu, Adaptive Gaussian/Mean, and Global thresholding.
  - **Sharpening**: Unsharp masking for faint fonts.
- **Extensible Provider Pattern (`IOcrProvider`)**: Reference implementation with native Tesseract 4.x LSTM; designed for future drop-in neural backends (ONNX, TensorRT, PaddleOCR, custom EDMO models).
- **Complete Document Graph Hierarchy**: `Document -> Page -> Block -> Line -> Token` with normalized confidence scores (0.0 to 1.0) and pixel bounding boxes (top-left origin).
- **Deterministic JSON Output**: Compliant with EDMO schema v0.1 supporting `full-json`, `compact-json`, and `text` output modes.
- **Batch Processing**: Directory processing with aggregate summary statistics and fault tolerance.
- **Production CLI & Clean C++ Library API**: Direct library embedding or command-line execution.
- **Performance Benchmarking & Synthetic Fixtures**: Synthetic test generator producing realistic academic transcripts, test suite, and latency benchmark suite.
- **Docker Support**: Multi-stage production container and dev container.

---

## Repository Structure

```text
edmo-di-ocr/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── .clang-format
├── .clang-tidy
│
├── cmake/
│   ├── Dependencies.cmake
│   └── CompilerWarnings.cmake
│
├── include/edmo/ocr/
│   ├── OcrEngine.hpp          # Primary C++ Library API
│   ├── OcrConfig.hpp          # Configuration & Profiles
│   ├── OcrResult.hpp          # Result Hierarchy & Timings
│   ├── OcrOptions.hpp         # Provider Options
│   ├── Document.hpp           # Document Domain Model
│   ├── Page.hpp               # Page Domain Model
│   ├── Image.hpp              # OpenCV Mat RAII Abstraction
│   ├── BoundingBox.hpp        # Geometry & Intersections
│   ├── OcrToken.hpp           # Word/Token Structure
│   ├── OcrLine.hpp            # Line Structure
│   ├── OcrBlock.hpp           # Block/Paragraph Structure
│   ├── IOcrProvider.hpp       # Pluggable OCR Backend Interface
│   ├── IPreprocessor.hpp      # Preprocessing Pipeline Interface
│   ├── IPdfRenderer.hpp       # PDF Rendering Interface
│   ├── IOrientationDetector.hpp # Orientation Interface
│   ├── Result.hpp             # Value-or-Error Monad
│   ├── ErrorCode.hpp          # Structured Error Codes
│   └── Types.hpp              # Common Enums & Types
│
├── src/
│   ├── core/                  # Engine Orchestrator, Config, Document
│   ├── preprocessing/         # Deskew, Orientation, Denoise, CLAHE, Binarize
│   ├── pdf/                   # Poppler PDF Renderer
│   ├── ocr/                   # Tesseract Provider & Model Factory
│   ├── output/                # JSON & Plaintext Serializers
│   ├── logging/               # spdlog Integration
│   └── utils/                 # FileUtils, ImageUtils, Timer
│
├── apps/edmo-ocr/             # CLI Application
├── benchmarks/                # Performance Benchmark Tool
├── examples/                  # Basic and Batch C++ API Examples
├── configs/                   # Default & Transcript Profile Configurations
├── tests/                     # Unit, Integration, and Golden Schema Tests
├── scripts/                   # build.sh, test.sh, benchmark.sh
├── docker/                    # Production and Development Dockerfiles
└── docs/                      # Architecture, Pipeline, Schema, and Dev docs
```

---

## Quick Start

### 1. Requirements

Install required dependencies on Ubuntu / Debian:

```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake pkg-config \
    libtesseract-dev tesseract-ocr-eng tesseract-ocr-osd \
    libleptonica-dev libpoppler-cpp-dev
```

### 2. Build

```bash
./scripts/build.sh Release
```

### 3. Run Single Document OCR

```bash
# Process image or PDF with full JSON output
./build/apps/ocr-engine/ocr-engine tests/fixtures/simple-text.png --format full-json

# Process academic transcript PDF and save output to result.json
./build/apps/ocr-engine/ocr-engine tests/fixtures/transcript-like.pdf \
    --preprocess-profile transcript \
    --dpi 300 \
    --output result.json
```

### 4. Run Batch Processing

```bash
./build/apps/ocr-engine/ocr-engine ./tests/fixtures --batch --output ./results
```

---

## CLI Usage & Options

```text
Usage:
  edmo-ocr <input-file> [options]
  edmo-ocr <input-dir> --batch [options]

Options:
  -i, --input <path>               Input image/PDF file or directory
  -o, --output <path>              Output file destination or directory (for batch)
  -f, --format <format>            Output format: full-json, compact-json, text (default: full-json)
  -l, --language <lang>            OCR recognition language(s), e.g. eng, eng+fra (default: eng)
  -d, --dpi <int>                  Rendering DPI for PDF inputs: 150, 200, 300, 400, 600 (default: 300)
  -p, --preprocess-profile <name>  Preprocessing profile: default, clean-document, scanned-document, low-quality-scan, transcript
      --deskew                     Enable automatic skew angle detection and correction (default)
      --no-deskew                  Disable automatic skew correction
      --orientation                Enable 0/90/180/270 degree orientation detection (default)
      --no-orientation             Disable automatic orientation detection
  -c, --confidence <float>         Minimum token confidence threshold 0.0 - 1.0 (default: 0.0)
      --provider <name>            OCR Provider backend: tesseract (default)
      --tessdata-dir <path>        Custom tessdata language directory
      --config <path>              Load JSON configuration file
      --batch                      Enable batch processing mode on input directory
      --debug                      Enable debug mode and output intermediate image pipeline steps
      --verbose                    Enable verbose logging output to stderr
  -v, --version                    Display version and build information
  -h, --help                       Show this help message
```

---

## C++ Library API

```cpp
#include <ocr/OcrEngine.hpp>
#include <iostream>

int main() {
    // Load transcript-optimized configuration profile
    ocr::OcrConfig config = ocr::OcrConfig::getProfile("transcript");
    ocr::OcrEngine engine(config);

    // Process document
    auto result = engine.process("transcript.pdf");

    if (result) {
        std::cout << "Success! Confidence: " << (result->confidence * 100.0f) << "%\n";
        std::cout << "Total Time: " << result->timing.totalTimeMs << " ms\n";
        std::cout << result->toJson(ocr::OutputFormat::FullJson).dump(2) << "\n";
    } else {
        std::cerr << "Error: " << result.error().toString() << "\n";
    }
}
```

---

## Running Tests & Benchmarks

```bash
# Run unit & integration test suite
./scripts/test.sh

# Run benchmark suite
./scripts/benchmark.sh tests/fixtures/transcript-like.pdf 5
```

---

## Docker Support

### Build & Run Container

```bash
# Build production image
docker build -t edmo-di-ocr -f docker/Dockerfile .

# Process document via container
docker run --rm -v $(pwd)/tests/fixtures:/data edmo-di-ocr /data/transcript-like.pdf
```

---

## Documentation

- [Architecture Guide](docs/architecture.md)
- [Pipeline Deep-Dive](docs/pipeline.md)
- [Output Schema v0.1 Specification](docs/output-schema.md)
- [Developer Guide & Adding Providers](docs/development.md)

---

## License

Licensed under the [Apache License, Version 2.0](LICENSE).
