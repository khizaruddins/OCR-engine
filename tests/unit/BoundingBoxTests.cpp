#include <gtest/gtest.h>
#include <edmo/ocr/BoundingBox.hpp>

using namespace edmo::ocr;

TEST(BoundingBoxTests, DefaultConstructor) {
    BoundingBox bbox;
    EXPECT_EQ(bbox.x, 0);
    EXPECT_EQ(bbox.y, 0);
    EXPECT_EQ(bbox.width, 0);
    EXPECT_EQ(bbox.height, 0);
    EXPECT_TRUE(bbox.empty());
    EXPECT_EQ(bbox.area(), 0);
}

TEST(BoundingBoxTests, ParameterizedConstructorAndCoordinates) {
    BoundingBox bbox(10, 20, 100, 50);
    EXPECT_EQ(bbox.left(), 10);
    EXPECT_EQ(bbox.top(), 20);
    EXPECT_EQ(bbox.right(), 110);
    EXPECT_EQ(bbox.bottom(), 70);
    EXPECT_EQ(bbox.area(), 5000);
    EXPECT_FALSE(bbox.empty());
}

TEST(BoundingBoxTests, ContainsPoint) {
    BoundingBox bbox(10, 20, 100, 50);
    EXPECT_TRUE(bbox.contains(10, 20));   // top-left corner
    EXPECT_TRUE(bbox.contains(50, 40));   // center
    EXPECT_TRUE(bbox.contains(109, 69));  // bottom-right interior
    EXPECT_FALSE(bbox.contains(110, 70)); // bottom-right edge boundary
    EXPECT_FALSE(bbox.contains(5, 20));   // left outside
    EXPECT_FALSE(bbox.contains(50, 100)); // bottom outside
}

TEST(BoundingBoxTests, ContainsBox) {
    BoundingBox outer(10, 10, 100, 100);
    BoundingBox inner(20, 20, 50, 50);
    BoundingBox overlapping(50, 50, 100, 100);

    EXPECT_TRUE(outer.contains(inner));
    EXPECT_FALSE(inner.contains(outer));
    EXPECT_FALSE(outer.contains(overlapping));
    EXPECT_TRUE(outer.contains(outer));
}

TEST(BoundingBoxTests, Intersections) {
    BoundingBox b1(10, 10, 50, 50); // [10..60, 10..60]
    BoundingBox b2(40, 40, 50, 50); // [40..90, 40..90]
    BoundingBox b3(100, 100, 50, 50);

    EXPECT_TRUE(b1.intersects(b2));
    EXPECT_FALSE(b1.intersects(b3));

    BoundingBox inter = b1.intersectionWith(b2);
    EXPECT_EQ(inter.x, 40);
    EXPECT_EQ(inter.y, 40);
    EXPECT_EQ(inter.width, 20);
    EXPECT_EQ(inter.height, 20);
    EXPECT_EQ(inter.area(), 400);

    BoundingBox noInter = b1.intersectionWith(b3);
    EXPECT_TRUE(noInter.empty());
}

TEST(BoundingBoxTests, UnionWith) {
    BoundingBox b1(10, 10, 20, 20); // [10..30, 10..30]
    BoundingBox b2(40, 50, 20, 20); // [40..60, 50..70]

    BoundingBox u = b1.unionWith(b2);
    EXPECT_EQ(u.x, 10);
    EXPECT_EQ(u.y, 10);
    EXPECT_EQ(u.width, 50);
    EXPECT_EQ(u.height, 60);
}

TEST(BoundingBoxTests, JsonSerialization) {
    BoundingBox original(15, 25, 120, 80);
    nlohmann::json j = original.toJson();

    EXPECT_EQ(j["x"], 15);
    EXPECT_EQ(j["y"], 25);
    EXPECT_EQ(j["width"], 120);
    EXPECT_EQ(j["height"], 80);

    BoundingBox parsed = BoundingBox::fromJson(j);
    EXPECT_EQ(original, parsed);
}
