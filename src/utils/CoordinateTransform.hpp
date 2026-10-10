#pragma once

#include <edmo/ocr/BoundingBox.hpp>
#include <edmo/ocr/OcrResult.hpp>

namespace edmo::ocr {

class CoordinateTransform {
public:
    /**
     * @brief Inverts a bounding box from a rotated image coordinate space back to the original image coordinate space.
     * @param rotatedBox Bounding box in the rotated image
     * @param rotationAngleDegrees Clockwise rotation angle applied to the original image (0, 90, 180, 270)
     * @param originalWidth Width of the unrotated original image
     * @param originalHeight Height of the unrotated original image
     */
    [[nodiscard]] static BoundingBox invertBoundingBox(
        const BoundingBox& rotatedBox,
        int rotationAngleDegrees,
        int originalWidth,
        int originalHeight
    ) noexcept;

    /**
     * @brief Inverts all bounding boxes in an OcrPageResult back to the original unrotated page coordinate space.
     */
    static void invertPageCoordinates(
        OcrPageResult& pageResult,
        int rotationAngleDegrees,
        int originalWidth,
        int originalHeight
    ) noexcept;
};

} // namespace edmo::ocr
