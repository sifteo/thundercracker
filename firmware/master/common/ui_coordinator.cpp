/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "macros.h"
#include "ui_coordinator.h"
#include "cube.h"
#include "cubeslots.h"
#include "machine.h"
#include "tasks.h"


UICoordinator::UICoordinator(uint32_t excludedTasks)
    : uiConnected(0), excludedTasks(excludedTasks), savedVBuf(0)
{
    avb.cube = _SYS_CUBE_ID_INVALID;
}


void UICoordinator::stippleCubes(_SYSCubeIDVector cv)
{
    if (!cv)
        return;

    // Must quiesce existing drawing, so we don't switch modes mid-frame
    CubeSlots::finishCubes(cv, excludedTasks);

    // Pause normal VRAM updates until restoreCubes()
    Atomic::Or(CubeSlots::vramPaused, cv);

    /*
     * Ask the CubeSlot to send a canned stipple packet. This puts
     * the cube into STAMP mode, set up to draw a black & clear
     * checkerboard pattern over the whole screen.
     *
     * It will poke the TOGGLE bit if a vbuf is attached, otherwise
     * it goes into CONTINUOUS mode.
     */
    Atomic::Or(CubeSlots::sendStipple, cv);

    /*
     * Wait for the stipple packets to be sent, and the VRAM flags
     * to be updated accordingly.
     */
    while (cv & CubeSlots::sendStipple & CubeSlots::sysConnected)
        idle();

    LOG(("Okay!!\n"));
}

void UICoordinator::restoreCubes(_SYSCubeIDVector cv)
{
    if (!cv)
        return;

    // Cancel stippling, if it's still queued
    Atomic::And(CubeSlots::sendStipple, ~cv);

    // If we're attached to this cube, detach and restore its usual VideoBuffer
    if (avb.cube != _SYS_CUBE_ID_INVALID && (cv & Intrinsic::LZ(avb.cube)))
        detach();

    // The cube may be in continuous mode still, if we were stippling.
    // Make sure we totally finish rendering the stipple before coming back.
    CubeSlots::finishCubes(cv, excludedTasks);

    // Resume sending normal VRAM updates
    Atomic::And(CubeSlots::vramPaused, ~cv);

    // Ask CubeSlots to zap the change maps and send a REFRESH event.
    CubeSlots::refreshCubes(cv);
}

_SYSCubeIDVector UICoordinator::connectCubes()
{
    _SYSCubeIDVector sysConnected = CubeSlots::sysConnected;
    _SYSCubeIDVector oldConnected = uiConnected;
    uiConnected = sysConnected;
    return sysConnected & ~oldConnected;
}

void UICoordinator::attachToCube(_SYSCubeID id)
{
    detach();

    ASSERT(id < _SYS_NUM_CUBE_SLOTS);
    _SYSCubeIDVector cv = Intrinsic::LZ(id);
    CubeSlot &cube = CubeSlots::instances[id];

    // Quiesce rendering before we go about swapping vbufs
    CubeSlots::finishCubes(cv, excludedTasks);

    /*
     * Now some slight magic... for the smoothest transition, we want
     * to copy over the SYSVideoBuffer flags and VRAM flags from the
     * old buffer (if any), but to init the rest of our buffer from
     * scratch.
     */

    VRAM::init(avb.vbuf);
    avb.vbuf.vram.mode = _SYS_VM_BG0_ROM;
    avb.vbuf.vram.first_line = 32;
    avb.vbuf.vram.num_lines = 64;

    savedVBuf = cube.getVBuf();

    if (savedVBuf) {
        avb.vbuf.flags = savedVBuf->flags;
        avb.vbuf.vram.flags = savedVBuf->vram.flags;
    }

    avb.cube = id;
    cube.setVideoBuffer(&avb.vbuf);
    Atomic::And(CubeSlots::vramPaused, ~cv);
}

void UICoordinator::paint()
{
    CubeSlots::paintCubes(Intrinsic::LZ(avb.cube), true, excludedTasks);
}

void UICoordinator::finish()
{
    CubeSlots::finishCubes(Intrinsic::LZ(avb.cube), excludedTasks);
}

void UICoordinator::detach()
{
    if (avb.cube == _SYS_CUBE_ID_INVALID)
        return;

    if (savedVBuf) {
        savedVBuf->flags = avb.vbuf.flags;
        VRAM::pokeb(*savedVBuf, offsetof(_SYSVideoRAM, flags), avb.vbuf.vram.flags);
    }

    CubeSlots::instances[avb.cube].setVideoBuffer(savedVBuf);
    avb.cube = _SYS_CUBE_ID_INVALID;
    savedVBuf = 0;
}

void UICoordinator::idle()
{
    Tasks::idle(excludedTasks);
}

