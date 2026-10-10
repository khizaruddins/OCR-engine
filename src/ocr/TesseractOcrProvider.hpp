#pragma once

#include <edmo/ocr/IOcrProvider.hpp>
#include <memory>

namespace tesseract {
class TessBaseAPI;
}

namespace edmo::ocr {

class TesseractOcrProvider : public IOcrProvider {
public:
    TesseractOcrProvider();
    ~TesseractOcrProvider() override;

    Result<OcrPageResult> process(
        const Image& image,
        const OcrOptions& options,
        int pageNumber = 1
    ) override;

    Result<std::string> processCell(
        const Image& cellImage,
        int psm = 7,
        const std::string& charWhitelist = "",
        float* outConfidence = nullptr
    ) override;

    [[nodiscard]] std::string name() const override {
        return "tesseract";
    }

    [[nodiscard]] std::string version() const override;

    [[nodiscard]] bool isThreadSafe() const override {
        return false; // TessBaseAPI instance-confined
    }

private:
    static std::string resolveTessdataDirectory(const std::string& customDir);

    std::unique_ptr<tesseract::TessBaseAPI> api_;
    std::string currentLang_;
    int currentOem_ = -1;
    std::string currentTessdataDir_;
};

} // namespace edmo::ocr
