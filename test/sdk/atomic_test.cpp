#include <gtest/gtest.h>
#include <sifteo/machine.h>

using namespace Sifteo;


TEST(AtomicTest, Int32LoadWorks) {
    int32_t val = INT32_MAX;
    EXPECT_EQ(INT32_MAX, Atomic::Load(val));
}

TEST(AtomicTest, UInt32LoadWorks) {
    uint32_t val = UINT32_MAX;
    EXPECT_EQ(UINT32_MAX, Atomic::Load(val));
}

TEST(AtomicTest, Int32StoreWorks) {
    int32_t dest = 0;
    int32_t val = INT32_MAX;
    Atomic::Store(dest, val);
    EXPECT_EQ(INT32_MAX, dest);
}

TEST(AtomicTest, UInt32StoreWorks) {
    uint32_t dest = 0;
    uint32_t val = UINT32_MAX;
    Atomic::Store(dest, val);
    EXPECT_EQ(UINT32_MAX, dest);
}

TEST(AtomicTest, AndWorks) {
    uint32_t dest = 0x000000FF;
    Atomic::And(dest, 0xFF0000F0);
    EXPECT_EQ(0x000000F0, dest);
}

TEST(AtomicTest, OrWorks) {
    uint32_t dest = 0x000000FF;
    Atomic::Or(dest, 0xFF0000F0);
    EXPECT_EQ(0xFF0000FF, dest);
}

TEST(AtomicTest, SetBitWorks) {
    uint32_t dest = 0x000000FF;
    Atomic::SetBit(dest, 18);
    EXPECT_EQ(0x000400FF, dest);
}

TEST(AtomicTest, ClearBitWorks) {
    uint32_t dest = 0x010000FF;
    Atomic::ClearBit(dest, 24);
    EXPECT_EQ(0x000000FF, dest);
}

TEST(AtomicTest, Int32AddWorks) {
    int32_t dest = 1032;
    Atomic::Add(dest, 314);
    EXPECT_EQ(1346, dest);
}

TEST(AtomicTest, UInt32AddWorks) {
    uint32_t dest = INT32_MAX;
    Atomic::Add(dest, 314);
    EXPECT_EQ(2147483961, dest);
}

// FIXME: Not sure the purpose of this function, so not writting tests at the moment.
/*
TEST(AtomicTest, SetLZWorks) {
    uint32_t dest = 0x220000FF;
    Atomic::SetLZ(dest, 15);
    
    fprintf(stdout, "SetLZ: 0x%08x", dest);
    
    EXPECT_EQ(0x000000FF, dest);
}
*/

// FIXME: Not sure the purpose of this function, so not writting tests at the moment.
/*
TEST(AtomicTest, ClearLZWorks) {
    uint32_t dest = 0x220000FF;
    Atomic::ClearLZ(dest, 15);
    
    fprintf(stdout, "ClearLZ: 0x%08x", dest);
    
    EXPECT_EQ(0x000000FF, dest);
}
*/
