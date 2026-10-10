#include "FileUtils.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <sstream>

namespace ocr {

static std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

Result<void> FileUtils::validateFile(const std::filesystem::path& path, size_t maxFileSizeMb) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return Result<void>::failure(ErrorCode::FileNotFound, "File does not exist", path.string());
    }

    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        return Result<void>::failure(ErrorCode::InvalidDocument, "Path is not a regular file", path.string());
    }

    auto sizeBytes = std::filesystem::file_size(path, ec);
    if (ec) {
        return Result<void>::failure(ErrorCode::InvalidDocument, "Failed to read file size: " + ec.message(), path.string());
    }

    if (sizeBytes == 0) {
        return Result<void>::failure(ErrorCode::FileEmpty, "File is empty (0 bytes)", path.string());
    }

    size_t maxBytes = maxFileSizeMb * 1024 * 1024;
    if (sizeBytes > maxBytes) {
        return Result<void>::failure(
            ErrorCode::FileTooLarge,
            "File size (" + std::to_string(sizeBytes) + " bytes) exceeds maximum limit (" +
                std::to_string(maxBytes) + " bytes)",
            path.string()
        );
    }

    if (!isSupportedExtension(path)) {
        return Result<void>::failure(
            ErrorCode::UnsupportedFormat,
            "Unsupported file extension: " + path.extension().string(),
            path.string()
        );
    }

    // Attempt to open and read first bytes to confirm permissions
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open() || !file.good()) {
        return Result<void>::failure(ErrorCode::InvalidDocument, "File cannot be opened for reading (permission denied or lock)", path.string());
    }

    return Result<void>::success();
}

DocumentType FileUtils::detectDocumentType(const std::filesystem::path& path) {
    std::string ext = toLower(path.extension().string());
    if (ext == ".pdf") {
        return DocumentType::Pdf;
    }
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tif" || ext == ".tiff" || ext == ".bmp") {
        return DocumentType::Image;
    }
    return DocumentType::Unknown;
}

std::string FileUtils::getMimeType(const std::filesystem::path& path) {
    std::string ext = toLower(path.extension().string());
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".tif" || ext == ".tiff") return "image/tiff";
    if (ext == ".bmp") return "image/bmp";
    return "application/octet-stream";
}

size_t FileUtils::getFileSize(const std::filesystem::path& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    return ec ? 0 : size;
}

bool FileUtils::isSupportedExtension(const std::filesystem::path& path) {
    return detectDocumentType(path) != DocumentType::Unknown;
}

std::vector<std::filesystem::path> FileUtils::listSupportedFiles(
    const std::filesystem::path& directoryPath,
    bool recursive
) {
    std::vector<std::filesystem::path> results;
    std::error_code ec;

    if (!std::filesystem::exists(directoryPath, ec) || !std::filesystem::is_directory(directoryPath, ec)) {
        return results;
    }

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath, ec)) {
            if (entry.is_regular_file() && isSupportedExtension(entry.path())) {
                results.push_back(entry.path());
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath, ec)) {
            if (entry.is_regular_file() && isSupportedExtension(entry.path())) {
                results.push_back(entry.path());
            }
        }
    }

    std::sort(results.begin(), results.end());
    return results;
}

static std::string generateRandomHex(size_t len) {
    static constexpr char hexChars[] = "0123456789abcdef";
    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, 15);
    std::string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        s += hexChars[dist(gen)];
    }
    return s;
}

std::string FileUtils::generateDocumentId() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    return "doc_" + std::to_string(now) + "_" + generateRandomHex(6);
}

std::string FileUtils::generateProcessingId() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    return "proc_" + std::to_string(now) + "_" + generateRandomHex(6);
}

} // namespace ocr
