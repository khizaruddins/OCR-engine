# EDMO DI-OCR Developer Guide

## 1. Development Prerequisites

- Modern Linux environment (Ubuntu 20.04+, Debian 11+, Fedora 36+)
- GCC 11+ or Clang 14+ supporting C++20
- CMake 3.22+
- Packages:
  ```bash
  sudo apt-get install -y build-essential cmake pkg-config \
      libtesseract-dev tesseract-ocr-eng tesseract-ocr-osd \
      libleptonica-dev libpoppler-cpp-dev
  ```

---

## 2. Building the Project

```bash
# Configure and build in Release mode
./scripts/build.sh Release

# Or manually via CMake:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## 3. Running Tests and Benchmarks

```bash
# Generate synthetic test fixtures and run all tests
./scripts/test.sh

# Run benchmarks
./scripts/benchmark.sh tests/fixtures/transcript-like.pdf 5
```

---

## 4. How to Add a New OCR Provider Backend

To implement a new OCR provider (e.g. `OnnxOcrProvider`, `PaddleOcrProvider`, or custom `EdmoOcrProvider`):

1. **Implement `IOcrProvider` interface**:
   Create `src/ocr/OnnxOcrProvider.hpp` and implement:
   ```cpp
   #include <edmo/ocr/IOcrProvider.hpp>

   namespace edmo::ocr {

   class OnnxOcrProvider : public IOcrProvider {
   public:
       Result<OcrPageResult> process(
           const Image& image,
           const OcrOptions& options,
           int pageNumber = 1
       ) override {
           // Run ONNX neural network inference
           // Construct and return OcrPageResult
       }

       std::string name() const override { return "onnx"; }
       std::string version() const override { return "1.16.0"; }
       bool isThreadSafe() const override { return true; }
   };

   }
   ```

2. **Register in `OcrProviderFactory.cpp`**:
   ```cpp
   std::unique_ptr<IOcrProvider> createOcrProvider(const OcrConfig& config) {
       if (config.ocr.provider == "onnx") {
           return std::make_unique<OnnxOcrProvider>();
       }
       return std::make_unique<TesseractOcrProvider>();
   }
   ```

3. No changes to CLI or orchestration are necessary.
