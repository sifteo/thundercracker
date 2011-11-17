#include <gtest/gtest.h>
#include "cube.h"

class CubeTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
        CubeSlot::disableCubes(0xFFFFFFFF);
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
    EXPECT_EQ(0x00000000, CubeSlot::vecEnabled);
    CubeSlot::enableCubes(0xE0000000);
    EXPECT_EQ(0xE0000000, CubeSlot::vecEnabled);
    CubeSlot::enableCubes(0x10000000);
    EXPECT_EQ(0xF0000000, CubeSlot::vecEnabled);
}

TEST_F(CubeTest, DisableCubesWorks) {
    EXPECT_EQ(0x00000000, CubeSlot::vecEnabled);
    CubeSlot::enableCubes(0xE0000000);
    EXPECT_EQ(0xE0000000, CubeSlot::vecEnabled);
    CubeSlot::disableCubes(0x40000000);
    EXPECT_EQ(0xA0000000, CubeSlot::vecEnabled);
}

TEST_F(CubeTest, CubeStateWhenEnabledWorks) {
    CubeSlot::enableCubes(0xE0000000);
    
    CubeSlot &cube = CubeSlot::getInstance(1);
    EXPECT_EQ(1, cube.id());
    EXPECT_EQ(0x40000000, cube.bit());
    EXPECT_TRUE(cube.enabled());
}
