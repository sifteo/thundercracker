#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>
#include <sifteo/machine.h>

using namespace Sifteo;


TEST(IntrinsicTest, POPCOUNTWorks) {
    EXPECT_EQ((uint32_t) 0, Intrinsic::POPCOUNT(0x00000000));
    EXPECT_EQ((uint32_t) 8, Intrinsic::POPCOUNT(0x88888888));
    EXPECT_EQ((uint32_t)16, Intrinsic::POPCOUNT(0xAAAAAAAA));
    EXPECT_EQ((uint32_t)32, Intrinsic::POPCOUNT(0xFFFFFFFF));
}

TEST(IntrinsicTest, CLZWorks) {
    EXPECT_EQ((uint32_t) 0, Intrinsic::CLZ(0x80000000));
    EXPECT_EQ((uint32_t) 8, Intrinsic::CLZ(0x00800000));
    EXPECT_EQ((uint32_t)16, Intrinsic::CLZ(0x00008000));
    EXPECT_EQ((uint32_t)24, Intrinsic::CLZ(0x00000080));
    EXPECT_EQ((uint32_t)32, Intrinsic::CLZ(0x00000000));
}

TEST(IntrinsicTest, LZWorks) {
    EXPECT_EQ((uint32_t)0x80000000, Intrinsic::LZ(0));
    EXPECT_EQ((uint32_t)0x40000000, Intrinsic::LZ(1));
    EXPECT_EQ((uint32_t)0x00000001, Intrinsic::LZ(31));
}

TEST(IntrinsicTest, RORWorks) {
    //fprintf(stdout, "ROR: 0x%08x", Intrinsic::ROR(0x4B4B4B4B, 7));
    EXPECT_EQ((uint32_t)0x96969696, Intrinsic::ROR(0x4B4B4B4B, 7));
}

TEST(IntrinsicTest, ROLWorks) {
    //fprintf(stdout, "ROL: 0x%08x", Intrinsic::ROL(0x4B4B4B4B, 7));
    EXPECT_EQ((uint32_t)0xA5A5A5A5, Intrinsic::ROL(0x4B4B4B4B, 7));
}
