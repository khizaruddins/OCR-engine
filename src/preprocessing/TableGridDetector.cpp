#include <edmo/ocr/preprocessing/TableGridDetector.hpp>
#include <edmo/ocr/ErrorCode.hpp>
#include "../logging/Logger.hpp"

#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

namespace edmo::ocr {

Result<std::vector<DetectedTableGrid>> TableGridDetector::detect(
    const Image& image,
    const TableGridDetectorConfig& config
) {
    if (image.empty()) {
        return Result<std::vector<DetectedTableGrid>>::failure(
            ErrorCode::PreprocessingFailed,
            "Input image is empty for table grid detection"
        );
    }

    if (!config.enabled) {
        return Result<std::vector<DetectedTableGrid>>::success({});
    }

    cv::Mat gray;
    if (image.channels() == 1) {
        gray = image.mat();
    } else if (image.channels() == 4) {
        cv::cvtColor(image.mat(), gray, cv::COLOR_BGRA2GRAY);
    } else {
        cv::cvtColor(image.mat(), gray, cv::COLOR_BGR2GRAY);
    }

    // 1. Inverted binarization: foreground (lines/text) is white (255), background is black (0)
    cv::Mat binary;
    cv::adaptiveThreshold(
        gray, binary, 255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV,
        15, 2
    );

    // 2. Morphological extraction of horizontal and vertical line structures
    int hKernelWidth = std::max(15, gray.cols / config.horizontalLineKernelDivisor);
    int vKernelHeight = std::max(15, gray.rows / config.verticalLineKernelDivisor);

    cv::Mat hKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(hKernelWidth, 1));
    cv::Mat vKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, vKernelHeight));

    cv::Mat hLines, vLines;
    cv::morphologyEx(binary, hLines, cv::MORPH_OPEN, hKernel);
    cv::morphologyEx(binary, vLines, cv::MORPH_OPEN, vKernel);

    // Combine lines to form complete table grids
    cv::Mat tableGrid = hLines | vLines;

    // Dilate slightly to bridge minor line disconnects
    cv::Mat dilatedGrid;
    cv::Mat bridgeKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(tableGrid, dilatedGrid, bridgeKernel);

    // 3. Find outer boundaries of all tables
    std::vector<std::vector<cv::Point>> outerContours;
    cv::findContours(dilatedGrid.clone(), outerContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<DetectedTableGrid> detectedTables;

    for (const auto& contour : outerContours) {
        cv::Rect tableRect = cv::boundingRect(contour);

        if (tableRect.width < config.minTableWidth || tableRect.height < config.minTableHeight) {
            continue;
        }

        cv::Mat roiGrid = dilatedGrid(tableRect);

        // Find internal cell contours within this table boundary
        std::vector<std::vector<cv::Point>> cellContours;
        cv::findContours(roiGrid.clone(), cellContours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        std::vector<cv::Rect> validCellRects;
        int maxCellArea = static_cast<int>(tableRect.area() * 0.90);

        for (const auto& cContour : cellContours) {
            cv::Rect cRect = cv::boundingRect(cContour);

            // Filter out noise or outer enclosing boundary itself
            if (cRect.width >= 20 && cRect.height >= 12 &&
                cRect.area() >= 240 && cRect.area() <= maxCellArea) {

                // Global coordinates on page
                cv::Rect globalRect(
                    tableRect.x + cRect.x,
                    tableRect.y + cRect.y,
                    cRect.width,
                    cRect.height
                );

                // Deduplicate with existing cells (IoU / containment check)
                bool duplicate = false;
                for (const auto& existing : validCellRects) {
                    cv::Rect intersection = globalRect & existing;
                    double iou = static_cast<double>(intersection.area()) /
                        (globalRect.area() + existing.area() - intersection.area());
                    if (iou > 0.60) {
                        duplicate = true;
                        break;
                    }
                }

                if (!duplicate) {
                    validCellRects.push_back(globalRect);
                }
            }
        }

        if (validCellRects.size() < 4) {
            continue;
        }

        // 4. Cluster cells into sorted rows and columns
        // Sort primarily by Y-coordinate
        std::sort(validCellRects.begin(), validCellRects.end(), [](const cv::Rect& a, const cv::Rect& b) {
            return a.y < b.y;
        });

        // Group into rows based on vertical overlap
        std::vector<std::vector<cv::Rect>> rowGroups;
        for (const auto& rect : validCellRects) {
            bool placed = false;
            int rectMidY = rect.y + rect.height / 2;

            for (auto& row : rowGroups) {
                int rowMidY = row.front().y + row.front().height / 2;
                int rowTol = std::max(8, row.front().height / 2);

                if (std::abs(rectMidY - rowMidY) <= rowTol) {
                    row.push_back(rect);
                    placed = true;
                    break;
                }
            }

            if (!placed) {
                rowGroups.push_back({rect});
            }
        }

        // Sort rows vertically and sort cells within each row horizontally
        std::sort(rowGroups.begin(), rowGroups.end(), [](const std::vector<cv::Rect>& a, const std::vector<cv::Rect>& b) {
            return a.front().y < b.front().y;
        });

        int maxCols = 0;
        for (auto& row : rowGroups) {
            std::sort(row.begin(), row.end(), [](const cv::Rect& a, const cv::Rect& b) {
                return a.x < b.x;
            });
            maxCols = std::max(maxCols, static_cast<int>(row.size()));
        }

        int rowCount = static_cast<int>(rowGroups.size());
        if (rowCount < config.minRows || maxCols < config.minCols) {
            continue;
        }

        DetectedTableGrid grid;
        grid.bbox = BoundingBox(tableRect.x, tableRect.y, tableRect.width, tableRect.height);
        grid.rowCount = rowCount;
        grid.colCount = maxCols;
        grid.hasPhysicalBorders = true;
        grid.confidence = 0.95f;

        for (int r = 0; r < rowCount; ++r) {
            const auto& row = rowGroups[r];
            for (size_t c = 0; c < row.size(); ++c) {
                const auto& cr = row[c];
                TableCell cell;
                cell.bbox = BoundingBox(cr.x, cr.y, cr.width, cr.height);
                cell.rowIndex = r;
                cell.colIndex = static_cast<int>(c);
                cell.isHeader = (r == 0);
                grid.cells.push_back(cell);
            }
        }

        detectedTables.push_back(std::move(grid));
    }

    EDMO_LOG_DEBUG("TableGridDetector identified {} table grids with {} cells total",
        detectedTables.size(),
        [&]() {
            size_t count = 0;
            for (const auto& t : detectedTables) count += t.cells.size();
            return count;
        }()
    );

    return Result<std::vector<DetectedTableGrid>>::success(std::move(detectedTables));
}

cv::Mat TableGridDetector::renderDebugVisualization(
    const cv::Mat& src,
    const std::vector<DetectedTableGrid>& tables
) {
    cv::Mat visual;
    if (src.channels() == 1) {
        cv::cvtColor(src, visual, cv::COLOR_GRAY2BGR);
    } else {
        visual = src.clone();
    }

    for (const auto& table : tables) {
        // Draw table boundary in Blue
        cv::rectangle(
            visual,
            cv::Rect(table.bbox.x, table.bbox.y, table.bbox.width, table.bbox.height),
            cv::Scalar(255, 0, 0),
            2
        );

        // Draw cells in Green (headers in Yellow)
        for (const auto& cell : table.cells) {
            cv::Scalar color = cell.isHeader ? cv::Scalar(0, 255, 255) : cv::Scalar(0, 255, 0);
            cv::rectangle(
                visual,
                cv::Rect(cell.bbox.x, cell.bbox.y, cell.bbox.width, cell.bbox.height),
                color,
                1
            );
        }
    }

    return visual;
}

} // namespace edmo::ocr
