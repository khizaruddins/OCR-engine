#pragma once

#include <ocr/BoundingBox.hpp>
#include <ocr/Image.hpp>
#include <ocr/Result.hpp>

#include <opencv2/core.hpp>
#include <vector>

namespace ocr {

/**
 * @brief Represents a single isolated cell within a detected table grid.
 */
struct TableCell {
    BoundingBox bbox;
    int rowIndex = 0;
    int colIndex = 0;
    bool isHeader = false;
};

/**
 * @brief Represents a table grid structure detected at the image level before OCR.
 */
struct DetectedTableGrid {
    BoundingBox bbox;
    int rowCount = 0;
    int colCount = 0;
    std::vector<TableCell> cells;
    bool hasPhysicalBorders = true;
    float confidence = 0.0f;
};

/**
 * @brief Configuration parameters for morphological table grid detection.
 */
struct TableGridDetectorConfig {
    bool enabled = true;
    int minTableWidth = 150;
    int minTableHeight = 80;
    int minRows = 2;
    int minCols = 2;
    int horizontalLineKernelDivisor = 30; // image width / 30
    int verticalLineKernelDivisor = 30;   // image height / 30
    int cellPadding = 3;
};

/**
 * @brief Algorithmic pre-OCR table grid and cell detector using OpenCV morphological operations.
 */
class TableGridDetector {
public:
    /**
     * @brief Detects table structures and isolates individual cell bounding boxes in an image.
     */
    static Result<std::vector<DetectedTableGrid>> detect(
        const Image& image,
        const TableGridDetectorConfig& config = {}
    );

    /**
     * @brief Generates an annotated debug visualization image highlighting detected tables and cells.
     */
    static cv::Mat renderDebugVisualization(
        const cv::Mat& src,
        const std::vector<DetectedTableGrid>& tables
    );
};

} // namespace ocr
