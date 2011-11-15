#include <gtest/gtest.h>
#include <sifteo/machine.h>

using namespace Sifteo;


TEST(IntrinsicTest, CLZWorks) {
    EXPECT_EQ((uint32_t) 0, Intrinsic::CLZ(0x80000000));
    EXPECT_EQ((uint32_t) 8, Intrinsic::CLZ(0x00800000));
    EXPECT_EQ((uint32_t)16, Intrinsic::CLZ(0x00008000));
    EXPECT_EQ((uint32_t)24, Intrinsic::CLZ(0x00000080));
    // FIXME: This doesn't seem right...
    EXPECT_EQ((uint32_t)31, Intrinsic::CLZ(0x00000000));
}
