#pragma once

#include <ocr/IPreprocessor.hpp>
#include "Deskew.hpp"
#include "OrientationDetector.hpp"

namespace ocr {

class ImagePreprocessor : public IPreprocessor {
public:
    ImagePreprocessor();
    ~ImagePreprocessor() override = default;

    Result<PreprocessResult> process(
        const Image& input,
        const PreprocessingConfig& config,
        const DebugContext& debugCtx = {}
    ) override;

private:
    std::unique_ptr<IOrientationDetector> orientationDetector_;

    void saveDebugImage(
        const Image& img,
        const DebugContext& ctx,
        const std::string& stepName
    );
};

} // namespace ocr
