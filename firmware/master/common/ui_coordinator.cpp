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
    : uiConnected(0), excludedTasks(excludedTasks),
      savedVBuf(0), stippleDeadline(0)
{
    memset(&avb, 0, sizeof avb);
    avb.cube = _SYS_CUBE_ID_INVALID;
}


void UICoordinator::stippleCubes(_SYSCubeIDVector cv)
{
    if (!cv)
        return;

    // Must quiesce existing drawing, so we don't switch modes mid-frame.
    // Note that we can't do this on cubes that are already paused.
    CubeSlots::finishCubes(cv & ~CubeSlots::vramPaused, excludedTasks);

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

    /*
     * Now, set a timer to let us guess when the stipple has finished
     * drawing. We can't rely on PaintControl for this, unfortunately,
     * since (a) not all cubes here necessarily have VideoBuffers
     * attached, and (b) the stipple command above just stomped all
     * over the cube's VRAM in a way that's tricky to fully account
     * for with specific hooks into PaintControl. We may or may not have
     * been able to issue a toggle, and the cube may or may not have been
     * already synchronized when we sent the toggle.
     *
     * And to add insult to injury, even if we did have a working
     * PaintController to point at this problem, stippling can be slower
     * than its frame rendering timeout, meaning that it wouldn't be
     * able to successfully re-synchronize in all cases anyway. So
     * we may as well do this ourselves.
     */
    stippleDeadline = SysTime::ticks() + SysTime::msTicks(200);
}

void UICoordinator::restoreCubes(_SYSCubeIDVector cv)
{
    if (!cv)
        return;

    // Cancel stippling, just in case it's still queued
    Atomic::And(CubeSlots::sendStipple, ~cv);

    // If we're attached to this cube, detach and restore its usual VideoBuffer
    if (avb.cube != _SYS_CUBE_ID_INVALID && (cv & Intrinsic::LZ(avb.cube)))
        detach();

    /*
     * Before unpausing VRAM updates, make sure we've given the stipple
     * enough time to render. If we were in CONTINUOUS mode, it's still
     * rendering and we're going to have a hard time resynchronizing.
     * Blah.
     */
    while (SysTime::ticks() < stippleDeadline)
        idle();

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

    // Wait for stipple to finish, if necessary
    while (SysTime::ticks() < stippleDeadline)
        idle();

    // Quiesce rendering before we go about swapping vbufs
    CubeSlots::finishCubes(cv & ~CubeSlots::vramPaused, excludedTasks);

    /*
     * Now some slight magic... for the smoothest transition, we want
     * to copy over the SYSVideoBuffer flags and VRAM flags from the
     * old buffer (if any), but to init the rest of our buffer from
     * scratch.
     */

    VRAM::init(avb.vbuf);
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
    if (isAttached())
        CubeSlots::paintCubes(Intrinsic::LZ(avb.cube), true, excludedTasks);
    else
        idle();
}

void UICoordinator::finish()
{
    if (isAttached())
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

void UICoordinator::setPanX(int x)
{
    VRAM::pokeb(avb.vbuf, offsetof(_SYSVideoRAM, bg0_x),
        umod(x, _SYS_VRAM_BG0_WIDTH * 8));
}

void UICoordinator::setPanY(int y)
{
    VRAM::pokeb(avb.vbuf, offsetof(_SYSVideoRAM, bg0_y),
        umod(y, _SYS_VRAM_BG0_WIDTH * 8));
}

void UICoordinator::idle()
{
    // We need to clear touch events manually, since we're
    // intentionally suppressing userspace event dispatch.
    CubeSlots::clearTouchEvents();

    Tasks::idle(excludedTasks);
}

bool UICoordinator::pollForAttach()
{
    /*
     * If we aren't attached, or we were attached to a disconnected cube,
     * attach to a new primary cube. Returns 'true' if we (re)attached.
     */

    if (isAttached() && !(Intrinsic::LZ(avb.cube) & CubeSlots::sysConnected)) {
        // Our attached cube disappeared!
        detach();
        ASSERT(!isAttached());
    }

    if (!isAttached() && uiConnected) {
        // Grab any connected cube
        attachToCube(Intrinsic::CLZ(uiConnected));
        return true;
    }

    return false;
}
