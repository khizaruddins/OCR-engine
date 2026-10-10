#pragma once

#include <algorithm>
#include <nlohmann/json.hpp>

namespace edmo::ocr {

/**
 * @brief Represents an axis-aligned rectangular bounding box.
 * Coordinate System:
 * - Origin (0,0) is at the top-left of the page/image.
 * - X increases to the right.
 * - Y increases downwards.
 */
struct BoundingBox {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    constexpr BoundingBox() noexcept = default;
    constexpr BoundingBox(int x_, int y_, int width_, int height_) noexcept
        : x(x_), y(y_), width(width_), height(height_) {}

    [[nodiscard]] constexpr int left() const noexcept { return x; }
    [[nodiscard]] constexpr int top() const noexcept { return y; }
    [[nodiscard]] constexpr int right() const noexcept { return x + width; }
    [[nodiscard]] constexpr int bottom() const noexcept { return y + height; }

    [[nodiscard]] constexpr int area() const noexcept {
        return (width > 0 && height > 0) ? (width * height) : 0;
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return width <= 0 || height <= 0;
    }

    [[nodiscard]] constexpr bool contains(int px, int py) const noexcept {
        return px >= x && px < right() && py >= y && py < bottom();
    }

    [[nodiscard]] constexpr bool contains(const BoundingBox& other) const noexcept {
        if (other.empty()) return true;
        if (empty()) return false;
        return other.x >= x && other.y >= y &&
               other.right() <= right() && other.bottom() <= bottom();
    }

    [[nodiscard]] constexpr bool intersects(const BoundingBox& other) const noexcept {
        if (empty() || other.empty()) return false;
        return x < other.right() && right() > other.x &&
               y < other.bottom() && bottom() > other.y;
    }

    [[nodiscard]] BoundingBox unionWith(const BoundingBox& other) const noexcept {
        if (empty()) return other;
        if (other.empty()) return *this;
        int minX = std::min(x, other.x);
        int minY = std::min(y, other.y);
        int maxX = std::max(right(), other.right());
        int maxY = std::max(bottom(), other.bottom());
        return BoundingBox(minX, minY, maxX - minX, maxY - minY);
    }

    [[nodiscard]] BoundingBox intersectionWith(const BoundingBox& other) const noexcept {
        if (!intersects(other)) return BoundingBox(0, 0, 0, 0);
        int minX = std::max(x, other.x);
        int minY = std::max(y, other.y);
        int maxX = std::min(right(), other.right());
        int maxY = std::min(bottom(), other.bottom());
        return BoundingBox(minX, minY, maxX - minX, maxY - minY);
    }

    constexpr bool operator==(const BoundingBox& other) const noexcept {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }

    constexpr bool operator!=(const BoundingBox& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"x", x},
            {"y", y},
            {"width", width},
            {"height", height}
        };
    }

    static BoundingBox fromJson(const nlohmann::json& j) {
        return BoundingBox(
            j.value("x", 0),
            j.value("y", 0),
            j.value("width", 0),
            j.value("height", 0)
        );
    }
};

} // namespace edmo::ocr
