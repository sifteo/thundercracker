/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "paintcontrol.h"
#include "tasks.h"
#include "radio.h"
#include "vram.h"
#include "cube.h"
#include "systime.h"

#define LOG_PREFIX  "PAINT[%p]: %12u us  "
#define LOG_PARAMS  this, unsigned(SysTime::ticks() / SysTime::usTicks(1))


/*
 * This object manages the somewhat complex asynchronous rendering pipeline.
 * We try to balance fast asynchronous rendering with slower but more deliberate
 * synchronous rendering.
 */

/*
 * System-owned VideoBuffer flag bits.
 *
 * Some of these flags are public, defined in the ABI. So far, the range
 * 0x0000FFFF is tentatively defined for public bits, while 0xFFFF0000 is
 * for these private bits. We make no guarantee about the meaning of these
 * bits, except that it's safe to initialize them to zero.
 */

// Public bits, defined in abi.h
//      _SYS_VBF_NEED_PAINT         (1 << 0)    // Request a paint operation

#define _SYS_VBF_DIRTY_RENDER       (1 << 16)   // Still rendering changed VRAM
#define _SYS_VBF_SYNC_ACK           (1 << 17)   // Frame ACK is synchronous (pendingFrames is 0 or 1)
#define _SYS_VBF_TRIGGER_ON_FLUSH   (1 << 18)   // Trigger a paint from vramFlushed()


/*
 * Frame rate control parameters:
 *
 * fpsLow --
 *    "Minimum" frame rate. If we're waiting more than this long
 *    for a frame to render, give up. Prevents us from getting wedged
 *    if a cube stops responding.
 *
 * fpsHigh --
 *    Maximum frame rate. Paint will always block until at least this
 *    long since the previous frame, in order to provide a global rate
 *    limit for the whole app.
 *
 * fpMax --
 *    Maximum number of pending frames to track in continuous mode.
 *    If we hit this limit, Paint() calls will block.
 *
 * fpMin --
 *    Minimum number of pending frames to track in continuous mode.
 *    If we go below this limit, we'll start ignoring acknowledgments.
 */

static const SysTime::Ticks fpsLow = SysTime::hzTicks(4);
static const SysTime::Ticks fpsHigh = SysTime::hzTicks(60);
static const int8_t fpMax = 5;
static const int8_t fpMin = -8;


void PaintControl::waitForPaint(CubeSlot *cube)
{
    /*
     * Wait until we're allowed to do another paint. Since our
     * rendering is usually not fully synchronous, this is not nearly
     * as strict as waitForFinish()!
     */

    DEBUG_LOG((LOG_PREFIX "------------------------\n", LOG_PARAMS));
    DEBUG_LOG((LOG_PREFIX "+waitForPaint() pend=%d\n",
        LOG_PARAMS, pendingFrames));

    SysTime::Ticks now;
    for (;;) {
        Atomic::Barrier();
        now = SysTime::ticks();

        // Watchdog expired? Give up waiting.
        if (now > paintTimestamp + fpsLow)
            break;

        // Wait for minimum frame rate AND for pending renders
        if (now > paintTimestamp + fpsHigh && pendingFrames <= fpMax)
            break;

        Tasks::work();
        Radio::halt();
    }

    /*
     * Can we opportunistically regain our synchronicity here?
     */

    _SYSVideoBuffer *vbuf = cube->getVBuf();
    if (vbuf && canMakeSynchronous(vbuf, now)) {
        makeSynchronous(vbuf);
        paintTimestamp = 0;
    }

    DEBUG_LOG((LOG_PREFIX "-waitForPaint(), expired=%d\n",
        LOG_PARAMS, now > paintTimestamp + fpsLow));
}

void PaintControl::triggerPaint(CubeSlot *cube, SysTime::Ticks timestamp)
{
    _SYSVideoBuffer *vbuf = cube->getVBuf();

    /*
     * We must always update paintTimestamp, even if this turned out
     * to be a no-op. An application which makes no changes to VRAM
     * but just calls paint() in a tight loop should iterate at the
     * 'fastPeriod' defined above.
     */
    paintTimestamp = timestamp;

    if (!vbuf)
        return;

    int32_t pending = Atomic::Load(pendingFrames);
    int32_t newPending = pending;

    DEBUG_LOG((LOG_PREFIX "+triggerPaint, pend=%d flags=%08x\n",
        LOG_PARAMS, pending, vbuf->flags));

    bool needPaint = (vbuf->flags & _SYS_VBF_NEED_PAINT) != 0;
    Atomic::And(vbuf->flags, ~_SYS_VBF_NEED_PAINT);

    /*
     * Keep pendingFrames above the lower limit. We make this
     * adjustment lazily, rather than doing it from inside the
     * ISR.
     */

    if (pending < fpMin)
        newPending = fpMin;

    /*
     * If we're in continuous rendering, we must count every single
     * Paint invocation for the purposes of loosely matching them with
     * acknowledged frames. This isn't a strict 1:1 mapping, but
     * it's used to close the loop on repaint speed.
     */

    if (needPaint) {
        newPending++;

        /*
         * There are multiple ways to enter continuous mode: vramFlushed()
         * can do so while handling a TRIGGER_ON_FLUSH flag, if we aren't
         * sync'ed by then. But we can't rely on this as our only way to
         * start rendering. If userspace is just pumping data into a VideoBuffer
         * like mad, and we can't stream it out over the radio quite fast enough,
         * we may not get a chance to enter vramFlushed() very often.
         *
         * So, the primary method for entering continuous mode is still as a
         * result of TRIGGER_ON_FLUSH. But as a backup, we'll enter it now
         * if we see frames stacking up in newPending.
         */

        if (newPending >= fpMax) {
            uint8_t vf = getFlags(vbuf);
            if (!(vf & _SYS_VF_CONTINUOUS)) {
                enterContinuous(vbuf, vf);
                setFlags(vbuf, vf);
            }
            newPending = fpMax;
        }

        if (!isContinuous(vbuf))
            Atomic::Or(vbuf->flags, _SYS_VBF_TRIGGER_ON_FLUSH);
    }

    // Atomically apply our changes to pendingFrames.
    Atomic::Add(pendingFrames, newPending - pending);

    DEBUG_LOG((LOG_PREFIX "-triggerPaint, pend=%d flags=%08x\n",
        LOG_PARAMS, newPending, vbuf->flags));
}

void PaintControl::waitForFinish(CubeSlot *cube)
{
    /*
     * Wait until all previous rendering has finished, and all of VRAM
     * has been updated over the radio.  Does *not* wait for any
     * minimum frame rate. If no rendering is pending, we return
     * immediately.
     *
     * Requires a valid attached video buffer.
     */

    _SYSVideoBuffer *vbuf = cube->getVBuf();
    ASSERT(vbuf);

    DEBUG_LOG((LOG_PREFIX "+waitForFinish(), flags=%08x\n", LOG_PARAMS, vbuf->flags));

    SysTime::Ticks now = SysTime::ticks();
    SysTime::Ticks deadline = now + fpsLow;

    // Disable continuous rendering now, if it was on.
    uint8_t vf = getFlags(vbuf);
    exitContinuous(vbuf, vf, now);
    setFlags(vbuf, vf);

    while (_SYS_VBF_DIRTY_RENDER & Atomic::Load(vbuf->flags)) {

        if (SysTime::ticks() > deadline) {
            DEBUG_LOG((LOG_PREFIX "waitForFinish() -- TIMED OUT\n", LOG_PARAMS));
            break;
        }

        Tasks::work();
        Radio::halt();
    }

    // We know we're sync'ed now.
    makeSynchronous(vbuf);

    DEBUG_LOG((LOG_PREFIX "-waitForFinish()\n", LOG_PARAMS));
}

void PaintControl::ackFrames(CubeSlot *cube, int32_t count)
{
    /*
     * One or more frames finished rendering on the cube.
     * Use this to update our pendingFrames accumulator.
     *
     * If we are _not_ in continuous rendering mode, and
     * we have synchronized our ACK bits with the cube's
     * TOGGLE bit, this means the frame has finished
     * rendering and we can clear the 'render' dirty bit.
     */
    
    pendingFrames -= count;

    _SYSVideoBuffer *vbuf = cube->getVBuf();
    if (vbuf) {
        uint32_t vf = getFlags(vbuf);

        if ((vf & _SYS_VF_CONTINUOUS) == 0 &&
            (vbuf->flags & _SYS_VBF_SYNC_ACK) != 0) {

            // Render is clean
            Atomic::And(vbuf->flags, ~_SYS_VBF_DIRTY_RENDER);
        }

        // Too few pending frames? Disable continuous mode.
        if (pendingFrames < fpMin) {
            uint8_t vf = getFlags(vbuf);
            exitContinuous(vbuf, vf, SysTime::ticks());
            setFlags(vbuf, vf);
        }

        DEBUG_LOG((LOG_PREFIX "ackFrames(%d), ack=%02x pend=%d flags=%08x\n",
            LOG_PARAMS, count, cube->getLastFrameACK(), pendingFrames, vbuf->flags));
    }
}

void PaintControl::vramFlushed(CubeSlot *cube)
{
    /*
     * Finished flushing VRAM out to the cubes. This is only called when
     * we've fully emptied our queue of pending radio transmissions, and
     * the cube's VRAM should match our local copy exactly.
     *
     * If we are in continuous rendering mode, this isn't really an
     * important event. But if we're in synchronous mode, this indicates
     * that everything in the VRAM dirty bit can now be tracked
     * by the RENDER dirty bit; in other words, all dirty VRAM has been
     * flushed, and we can start a clean frame rendering.
     */

    _SYSVideoBuffer *vbuf = cube->getVBuf();
    if (!vbuf)
        return;
    uint8_t vf = getFlags(vbuf);

    DEBUG_LOG((LOG_PREFIX "vramFlushed(), vf=%02x flags=%08x\n",
        LOG_PARAMS, vf, vbuf->flags));

    if (vbuf->flags & _SYS_VBF_TRIGGER_ON_FLUSH) {
        // Trying to trigger a render

        DEBUG_LOG((LOG_PREFIX "Triggering\n", LOG_PARAMS));

        if (cube->hasValidFrameACK() && (vbuf->flags & _SYS_VBF_SYNC_ACK)) {
            // We're sync'ed up. Trigger a one-shot render

            // Should never have SYNC_ACK set when in CONTINUOUS mode.
            ASSERT((vf & _SYS_VF_CONTINUOUS) == 0);

            setToggle(cube, vbuf, vf, SysTime::ticks());

        } else {
            /*
             * We're getting ahead of the cube. We'd like to trigger now, but
             * we're no longer in sync. So, enter continuous mode. This will
             * break synchronization, in the interest of keeping our speed up.
             */

             if (!(vf & _SYS_VF_CONTINUOUS))
                enterContinuous(vbuf, vf);
        }

        setFlags(vbuf, vf);
    }
}

void PaintControl::enterContinuous(_SYSVideoBuffer *vbuf, uint8_t &flags)
{
    DEBUG_LOG((LOG_PREFIX "enterContinuous\n", LOG_PARAMS));

    // Entering continuous mode; all synchronization goes out the window.
    Atomic::And(vbuf->flags, ~_SYS_VBF_SYNC_ACK);
    Atomic::Or(vbuf->flags, _SYS_VBF_DIRTY_RENDER);
    flags |= _SYS_VF_CONTINUOUS;
}

void PaintControl::exitContinuous(_SYSVideoBuffer *vbuf, uint8_t &flags,
    SysTime::Ticks timestamp)
{
    DEBUG_LOG((LOG_PREFIX "exitContinuous\n", LOG_PARAMS));

    // Exiting continuous mode; treat this as the last trigger point.
    if (flags & _SYS_VF_CONTINUOUS) {
        flags &= ~_SYS_VF_CONTINUOUS;
        triggerTimestamp = timestamp;
    }
}

bool PaintControl::isContinuous(_SYSVideoBuffer *vbuf)
{
    return (getFlags(vbuf) & _SYS_VF_CONTINUOUS) != 0;
}

void PaintControl::setToggle(CubeSlot *cube, _SYSVideoBuffer *vbuf,
    uint8_t &flags, SysTime::Ticks timestamp)
{
    DEBUG_LOG((LOG_PREFIX "setToggle\n", LOG_PARAMS));

    triggerTimestamp = timestamp;
    if (cube->getLastFrameACK() & 1)
        flags &= ~_SYS_VF_TOGGLE;
    else
        flags |= _SYS_VF_TOGGLE;

    // Propagate the bits...
    Atomic::Or(vbuf->flags, _SYS_VBF_DIRTY_RENDER);
    Atomic::And(vbuf->flags, ~_SYS_VBF_TRIGGER_ON_FLUSH);
}

void PaintControl::makeSynchronous(_SYSVideoBuffer *vbuf)
{
    DEBUG_LOG((LOG_PREFIX "makeSynchronous\n", LOG_PARAMS));

    pendingFrames = 0;
    Atomic::Or(vbuf->flags, _SYS_VBF_SYNC_ACK);
    Atomic::And(vbuf->flags, ~(_SYS_VBF_DIRTY_RENDER |
                               _SYS_VBF_TRIGGER_ON_FLUSH));
}

bool PaintControl::canMakeSynchronous(_SYSVideoBuffer *vbuf, SysTime::Ticks timestamp)
{
    return !isContinuous(vbuf) && timestamp > triggerTimestamp + fpsLow;
}
