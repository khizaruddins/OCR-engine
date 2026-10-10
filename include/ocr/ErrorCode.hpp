#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace edmo::ocr {

/**
 * @brief Structured error codes for all failure modes in the OCR engine.
 */
enum class ErrorCode {
    Success = 0,
    FileNotFound,
    FileTooLarge,
    FileEmpty,
    UnsupportedFormat,
    InvalidDocument,
    PdfRenderingFailed,
    ImageLoadFailed,
    PreprocessingFailed,
    OrientationDetectionFailed,
    DeskewFailed,
    OcrInitializationFailed,
    OcrProcessingFailed,
    SerializationFailed,
    InvalidConfiguration,
    OperationTimeout,
    InvalidJson,
    FileReadError,
    InternalError
};

/**
 * @brief Convert ErrorCode to string name.
 */
inline std::string_view errorCodeToString(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Success:
            return "Success";
        case ErrorCode::FileNotFound:
            return "FileNotFound";
        case ErrorCode::FileTooLarge:
            return "FileTooLarge";
        case ErrorCode::FileEmpty:
            return "FileEmpty";
        case ErrorCode::UnsupportedFormat:
            return "UnsupportedFormat";
        case ErrorCode::InvalidDocument:
            return "InvalidDocument";
        case ErrorCode::PdfRenderingFailed:
            return "PdfRenderingFailed";
        case ErrorCode::ImageLoadFailed:
            return "ImageLoadFailed";
        case ErrorCode::PreprocessingFailed:
            return "PreprocessingFailed";
        case ErrorCode::OrientationDetectionFailed:
            return "OrientationDetectionFailed";
        case ErrorCode::DeskewFailed:
            return "DeskewFailed";
        case ErrorCode::OcrInitializationFailed:
            return "OcrInitializationFailed";
        case ErrorCode::OcrProcessingFailed:
            return "OcrProcessingFailed";
        case ErrorCode::SerializationFailed:
            return "SerializationFailed";
        case ErrorCode::InvalidConfiguration:
            return "InvalidConfiguration";
        case ErrorCode::OperationTimeout:
            return "OperationTimeout";
        case ErrorCode::InvalidJson:
            return "InvalidJson";
        case ErrorCode::FileReadError:
            return "FileReadError";
        case ErrorCode::InternalError:
        default:
            return "InternalError";
    }
}

/**
 * @brief Rich structured error object containing code, descriptive message, and context.
 */
struct OcrError {
    ErrorCode code = ErrorCode::InternalError;
    std::string message;
    std::string path;
    std::optional<int> pageNumber;

    [[nodiscard]] std::string toString() const {
        std::string res = std::string(errorCodeToString(code)) + ": " + message;
        if (!path.empty()) {
            res += " (path: " + path + ")";
        }
        if (pageNumber.has_value()) {
            res += " [page " + std::to_string(*pageNumber) + "]";
        }
        return res;
    }
};

} // namespace edmo::ocr
