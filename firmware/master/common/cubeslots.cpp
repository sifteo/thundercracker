/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "cubeslots.h"
#include "cube.h"
#include "neighborslot.h"
#include "machine.h"
#include "tasks.h"
#include "radio.h"
#include "event.h"
#include "cubeconnector.h"
#include "pause.h"

CubeSlot CubeSlots::instances[_SYS_NUM_CUBE_SLOTS];

_SYSCubeIDVector CubeSlots::sysConnected = 0;
_SYSCubeIDVector CubeSlots::disconnectFlag = 0;
_SYSCubeIDVector CubeSlots::userConnected = 0;
_SYSCubeIDVector CubeSlots::sendShutdown = 0;
_SYSCubeIDVector CubeSlots::sendStipple = 0;
_SYSCubeIDVector CubeSlots::vramPaused = 0;
_SYSCubeIDVector CubeSlots::touch = 0;
_SYSCubeIDVector CubeSlots::waitingOnCubes = 0;
_SYSCubeIDVector CubeSlots::pendingHop = 0;

BitVector<SysLFS::NUM_PAIRINGS> CubeSlots::pairConnected;

_SYSCubeID CubeSlots::minUserCubes = 0;
_SYSCubeID CubeSlots::maxUserCubes = _SYS_NUM_CUBE_SLOTS;


void CubeSlots::setCubeRange(unsigned minimum, unsigned maximum)
{
    /*
     * update our counts asap, as the radio may preempt us,
     * and we'd like to be up to date for it, especially with respect to
     * the shutdown message that may get pended for additional cubes.
     */

    minUserCubes = minimum;
    maxUserCubes = maximum;

    // if we have too many cubes connected, shutdown the extras
    int excessCount = numConnected() - maximum;
    if (excessCount > 0) {

        // sysConnected isn't updated until the cubes actually get shut down, later
        _SYSCubeIDVector connected = sysConnected;

        while (excessCount--) {
            // shutdown the highest cube IDs
            uint32_t trailingZeros = Intrinsic::CTZ(connected);
            connected &= ~(1 << trailingZeros);

            uint32_t excessID = 31 - trailingZeros;
            ASSERT(excessID < _SYS_NUM_CUBE_SLOTS);
            Atomic::Or(sendShutdown, Intrinsic::LZ(excessID));
        }
    }

    // ensure reconnection is enabled if we have room, or disabled if we don't
    if (connectionSlotsAvailable())
        CubeConnector::enableReconnect();
    else
        CubeConnector::disableReconnect();

    // do we have enough cubes to continue?
    if (belowCubeRange())
        Pause::mainLoop(Pause::ModeCubeRange);
}

void CubeSlots::paintCubes(_SYSCubeIDVector cv, bool wait, uint32_t excludedTasks)
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

    if (wait) {
        _SYSCubeIDVector waitVec = cv;
        while (waitVec) {
            _SYSCubeID id = Intrinsic::CLZ(waitVec);
            CubeSlots::instances[id].waitForPaint(excludedTasks);
            waitVec ^= Intrinsic::LZ(id);
        }
    }

    SysTime::Ticks timestamp = SysTime::ticks();

    _SYSCubeIDVector paintVec = cv;
    while (paintVec) {
        _SYSCubeID id = Intrinsic::CLZ(paintVec);
        CubeSlots::instances[id].triggerPaint(timestamp);
        paintVec ^= Intrinsic::LZ(id);
    }
}

void CubeSlots::finishCubes(_SYSCubeIDVector cv, uint32_t excludedTasks)
{
    /*
     * Wait for rendering to finish on all cubes.
     *
     * Unlike paint(), finish may involve each cube being in a different
     * phase of rendering and require different inputs. To wait for each
     * cube concurrently instead of serially, we manage the main iteration
     * loop here and poll each cube in turn. Each poll operation may cause
     * state changes which get that cube closer to finishing.
     */

    // Start by keeping all of these cubes awake
    Atomic::Or(waitingOnCubes, cv);

    _SYSCubeIDVector beginVec = cv;
    while (beginVec) {
        _SYSCubeID id = Intrinsic::CLZ(beginVec);
        CubeSlots::instances[id].beginFinish();
        beginVec ^= Intrinsic::LZ(id);
    }

    for (;;) {
        SysTime::Ticks now = SysTime::ticks();
        _SYSCubeIDVector pollVec = cv;
        bool finished = true;

        while (pollVec) {
            _SYSCubeID id = Intrinsic::CLZ(pollVec);
            _SYSCubeIDVector bit = Intrinsic::LZ(id);
            pollVec ^= bit;

            if (CubeSlots::instances[id].pollForFinish(now)) {
                // This cube is done. Don't need to poll it any more.
                cv ^= bit;
                Atomic::And(waitingOnCubes, ~bit);
            } else {
                // Still waiting on at least one cube
                finished = false;
            }
        }
    
        if (finished)
            break;

        Tasks::idle(excludedTasks);
    }
}

void CubeSlots::refreshCubes(_SYSCubeIDVector cv)
{
    /*
     * For a set of cubes that we've monkeyed with behind the back of whatever
     * userspace app is running, go through and zap the change maps on their
     * video buffers, if any, and queue up REFRESH events for userspace to handle.
     */

    while (cv) {
        _SYSCubeID id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);

        _SYSVideoBuffer *vbuf = CubeSlots::instances[id].getVBuf();
        if (vbuf) {
            VRAM::init(*vbuf);
        }

        Event::setCubePending(Event::PID_CUBE_REFRESH, id);
    }
}

void CubeSlots::clearTouchEvents()
{
    _SYSCubeIDVector cv = CubeSlots::touch;
    while (cv) {
        unsigned id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);
        CubeSlots::instances[id].clearTouchEvent();
    }
}

void CubeSlots::disconnectCubes(_SYSCubeIDVector cv)
{
    while (cv) {
        unsigned id = Intrinsic::CLZ(cv);
        cv ^= Intrinsic::LZ(id);
        CubeSlots::instances[id].disconnect();
    }
}
