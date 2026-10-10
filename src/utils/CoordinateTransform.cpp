#include "CoordinateTransform.hpp"

#include <algorithm>

namespace edmo::ocr {

BoundingBox CoordinateTransform::invertBoundingBox(
    const BoundingBox& rotatedBox,
    int rotationAngleDegrees,
    int originalWidth,
    int originalHeight
) noexcept {
    int normAngle = (rotationAngleDegrees % 360 + 360) % 360;
    if (normAngle == 0) {
        return rotatedBox;
    }

    int rx = rotatedBox.x;
    int ry = rotatedBox.y;
    int rw = rotatedBox.width;
    int rh = rotatedBox.height;

    int ox = 0;
    int oy = 0;
    int ow = 0;
    int oh = 0;

    switch (normAngle) {
        case 90:
            // Rotated 90° CW:
            // x_orig = y_rot
            // y_orig = H_orig - (x_rot + w_rot)
            // w_orig = h_rot
            // h_orig = w_rot
            ox = ry;
            oy = originalHeight - (rx + rw);
            ow = rh;
            oh = rw;
            break;

        case 180:
            // Rotated 180°:
            // x_orig = W_orig - (x_rot + w_rot)
            // y_orig = H_orig - (y_rot + h_rot)
            // w_orig = w_rot
            // h_orig = h_rot
            ox = originalWidth - (rx + rw);
            oy = originalHeight - (ry + rh);
            ow = rw;
            oh = rh;
            break;

        case 270:
            // Rotated 270° CW (90° CCW):
            // x_orig = W_orig - (y_rot + h_rot)
            // y_orig = x_rot
            // w_orig = h_rot
            // h_orig = w_rot
            ox = originalWidth - (ry + rh);
            oy = rx;
            ow = rh;
            oh = rw;
            break;

        default:
            return rotatedBox;
    }

    // Clamp to original dimensions
    ox = std::clamp(ox, 0, originalWidth);
    oy = std::clamp(oy, 0, originalHeight);
    ow = std::clamp(ow, 0, originalWidth - ox);
    oh = std::clamp(oh, 0, originalHeight - oy);

    return BoundingBox(ox, oy, ow, oh);
}

void CoordinateTransform::invertPageCoordinates(
    OcrPageResult& pageResult,
    int rotationAngleDegrees,
    int originalWidth,
    int originalHeight
) noexcept {
    int normAngle = (rotationAngleDegrees % 360 + 360) % 360;
    if (normAngle == 0) {
        return;
    }

    pageResult.width = originalWidth;
    pageResult.height = originalHeight;

    for (auto& block : pageResult.blocks) {
        block.bbox = invertBoundingBox(block.bbox, normAngle, originalWidth, originalHeight);
        for (auto& line : block.lines) {
            line.bbox = invertBoundingBox(line.bbox, normAngle, originalWidth, originalHeight);
            for (auto& token : line.tokens) {
                token.bbox = invertBoundingBox(token.bbox, normAngle, originalWidth, originalHeight);
            }
        }
    }
}

} // namespace edmo::ocr
