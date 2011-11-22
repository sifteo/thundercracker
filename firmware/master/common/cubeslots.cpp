#include "cubeslots.h"
#include "cube.h"
#include "neighbors.h"
#include <sifteo/machine.h>

#if defined (BUILD_UNIT_TEST) && defined (UNIT_TEST_RADIO)
  #include "mockcube.h"
  #define CubeSlot MockCubeSlot
#endif

using namespace Sifteo;


CubeSlot CubeSlots::instances[_SYS_NUM_CUBE_SLOTS];


/*
 * Slot instances
 */
_SYSCubeIDVector CubeSlots::vecEnabled = 0;
_SYSCubeIDVector CubeSlots::vecConnected = 0;
_SYSCubeIDVector CubeSlots::flashResetWait = 0;
_SYSCubeIDVector CubeSlots::flashResetSent = 0;
_SYSCubeIDVector CubeSlots::flashACKValid = 0;
_SYSCubeIDVector CubeSlots::frameACKValid = 0;
_SYSCubeIDVector CubeSlots::neighborACKValid = 0;


_SYSCubeID CubeSlots::minCubes = 0;
_SYSCubeID CubeSlots::maxCubes = _SYS_NUM_CUBE_SLOTS;

void CubeSlots::solicitCubes(_SYSCubeID min, _SYSCubeID max) {
	minCubes = min;
	maxCubes = max;
}

void CubeSlots::enableCubes(_SYSCubeIDVector cv) {
    NeighborSlot::resetSlots(cv);
    Sifteo::Atomic::Or(CubeSlots::vecEnabled, cv);
}

void CubeSlots::disableCubes(_SYSCubeIDVector cv) {
    Sifteo::Atomic::And(CubeSlots::vecEnabled, ~cv);
    Sifteo::Atomic::And(CubeSlots::flashResetWait, ~cv);
    Sifteo::Atomic::And(CubeSlots::flashResetSent, ~cv);
    Sifteo::Atomic::And(CubeSlots::flashACKValid, ~cv);
    Sifteo::Atomic::And(CubeSlots::neighborACKValid, ~cv);
    NeighborSlot::resetSlots(cv);
    NeighborSlot::resetPairs(cv);
    // TODO: if any of the cubes in cv are currently part of a
    // neighbor-pair with any cubes that are still active, those
    // active cubes neeed to remove their now-defunct neighbors
}

void CubeSlots::connectCubes(_SYSCubeIDVector cv) {
    Sifteo::Atomic::Or(CubeSlots::vecConnected, cv);
}

void CubeSlots::disconnectCubes(_SYSCubeIDVector cv) {
    Sifteo::Atomic::And(CubeSlots::vecConnected, ~cv);
}


void CubeSlots::paintCubes(_SYSCubeIDVector cv)
{
    /*
     * If a previous repaint is still in progress, wait for it to
     * finish. Then trigger a repaint on all cubes that need one.
     *
     * Since we always send VRAM data to the radio in order of
     * increasing address, having the repaint trigger (vram.flags) at
     * the end of memory guarantees that the remainder of VRAM will
     * have already been sent by the time the cube gets the trigger.
     *
     * Why does this operate on a cube vector? Because we want to
     * trigger all cubes at close to the same time. So, we first wait
     * for all cubes to finish their last paint, then we trigger all
     * cubes.
     */

    _SYSCubeIDVector waitVec = cv;
    while (waitVec) {
        _SYSCubeID id = Intrinsic::CLZ(waitVec);
        CubeSlots::instances[id].waitForPaint();
        waitVec ^= Intrinsic::LZ(id);
    }

    SysTime::Ticks timestamp = SysTime::ticks();

    _SYSCubeIDVector paintVec = cv;
    while (paintVec) {
        _SYSCubeID id = Intrinsic::CLZ(paintVec);
        CubeSlots::instances[id].triggerPaint(timestamp);
        paintVec ^= Intrinsic::LZ(id);
    }
}

void CubeSlots::finishCubes(_SYSCubeIDVector cv)
{
    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        CubeSlots::instances[id].waitForFinish();
        cv ^= Intrinsic::LZ(id);
    }
}
