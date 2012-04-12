#include <gtest/gtest.h>
#include "rect.h"

TEST(RectTest, containsReturnsTrueForPointsInside) {
    Rect r = Rect(1.0f,2.0f, 3.0f,4.0f);
    Float2 pt = Vec2(1.1f, 2.1f);
    EXPECT_TRUE(r.contains(pt) );

    pt = Vec2(3.9, 5.9);
    EXPECT_TRUE(r.contains(pt) );
}

TEST(RectTest, containsReturnsTrueForPointsOnEdge) {
    Rect r = Rect(1.0f,2.0f, 3.0f,4.0f);
    Float2 pt = Vec2(1.0f, 2.0f);
    EXPECT_TRUE(r.contains(pt) );

    pt = Vec2(4.0f, 6.0f);
    EXPECT_TRUE(r.contains(pt) );
}

TEST(RectTest, containsReturnsFalseForPointsOutside) {
    Rect r = Rect(1.0f,2.0f, 3.0f,4.0f);
    Float2 pt;
    pt = Vec2(0.99f, 1.99f);
    EXPECT_FALSE(r.contains( pt ));

    pt = Vec2(4.01f, 6.01f);
    EXPECT_FALSE(r.contains(pt));
}

TEST(RectTest, isTouchingReturnsTrueForOvelappingRects){
    Rect r1 = Rect(0.0f,0.0f, 3.0f,4.0f);
    Rect r2 = Rect(1.0f,0.0f, 2.0f, 2.0f);
    EXPECT_TRUE(r1.isTouching(r2));
    EXPECT_TRUE(r2.isTouching(r1));
}

TEST(RectTest, isTouchingReturnsFalseForAdjacentRects){
    Rect r1 = Rect(0.0f,0.0f, 3.0f,4.0f);
    Rect r2 = Rect(3.0f,0.0f, 2.0f, 2.0f);
    EXPECT_FALSE(r1.isTouching(r2));
    EXPECT_FALSE(r2.isTouching(r1));
}

TEST(RectTest, isTouchingReturnsFalseForSeparateRects){
    Rect r1 = Rect(0.0f,0.0f, 1.0f,1.0f);
    Rect r2 = Rect(3.0f,0.0f, 1.0f, 1.0f);
    EXPECT_FALSE(r1.isTouching(r2));
    EXPECT_FALSE(r2.isTouching(r1));
}
