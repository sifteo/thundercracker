#include <gtest/gtest.h>
#include "cube.h"


class CubeTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
        CubeSlots::disableCubes(0xFFFFFFFF);
    }

    //virtual void TearDown() {}
};



TEST_F(CubeTest, GetInstanceWorks) {
    for (int i = 0; i < _SYS_NUM_CUBE_SLOTS; i++) {
        CubeSlot &cube = CubeSlot::getInstance(i);
        EXPECT_EQ(i, cube.id());
    }
    
    // FIXME: accessing instances directly is causing problems, possibly due to linking wrong
    //        with the master-sim.a archive.  Need to fix the makefile.
    //for (int i = 0; i < _SYS_NUM_CUBE_SLOTS; i++) {
    //    CubeSlot &cube = CubeSlot::instances[i];
    //    EXPECT_EQ(i, cube.id());
    //}
}

TEST_F(CubeTest, EnableCubesWorks) {
    EXPECT_EQ((_SYSCubeIDVector)0x00000000, CubeSlots::vecEnabled);
    CubeSlots::enableCubes(0xE0000000);
    EXPECT_EQ((_SYSCubeIDVector)0xE0000000, CubeSlots::vecEnabled);
    CubeSlots::enableCubes(0x10000000);
    EXPECT_EQ((_SYSCubeIDVector)0xF0000000, CubeSlots::vecEnabled);
}

TEST_F(CubeTest, DisableCubesWorks) {
    EXPECT_EQ((_SYSCubeIDVector)0x00000000, CubeSlots::vecEnabled);
    CubeSlots::enableCubes(0xE0000000);
    EXPECT_EQ((_SYSCubeIDVector)0xE0000000, CubeSlots::vecEnabled);
    CubeSlots::disableCubes(0x40000000);
    EXPECT_EQ((_SYSCubeIDVector)0xA0000000, CubeSlots::vecEnabled);
}

TEST_F(CubeTest, CubeStateWhenEnabledWorks) {
    CubeSlots::enableCubes(0xE0000000);
    
    CubeSlot &cube = CubeSlot::getInstance(1);
    EXPECT_EQ(1, cube.id());
    EXPECT_EQ((_SYSCubeIDVector)0x40000000, cube.bit());
    EXPECT_TRUE(cube.enabled());
}

// FIXME: This is broken
/*
TEST_F(CubeTest, RadioProduceWorks) {
    PacketTransmission ptx;
    CubeSlot &cube = CubeSlot::getInstance(1);
    cube.radioProduce(ptx);
    
    // TODO: test addressing/channel schemes correctly when support for them is more fully baked.
    EXPECT_EQ(ptx.dest->channel, 0x02);
    EXPECT_EQ(ptx.dest->id[0], 1);
    
    // TODO: Set various bits on the cube to fully test the radio codec.
}
*/
