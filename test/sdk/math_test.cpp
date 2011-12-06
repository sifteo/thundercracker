#include <gtest/gtest.h>
#include <sifteo/math.h>

using namespace Sifteo;


TEST(MathTest, ClampWorks) {
    EXPECT_EQ((uint32_t) 10, Math::clamp<uint32_t>(5, 10, 100));
    EXPECT_EQ((uint32_t) 50, Math::clamp<uint32_t>(50, 10, 100));
    EXPECT_EQ((uint32_t)100, Math::clamp<uint32_t>(105, 10, 100));
}
