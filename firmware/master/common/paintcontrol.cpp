/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "paintcontrol.h"
#include "tasks.h"
#include "radio.h"
#include "vram.h"
#include "cube.h"

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
 *
 * The 'dirty' bits are used to propagate dirty-state, starting with
 * _SYS_VBF_DIRTY_VRAM, through the whole paint pipeline:
 *
 *   1      -> VRAM    VideoBuffer lock() operation
 *   VRAM   -> RENDER  Finished streaming (cm16==0) and render triggered.
 *   RENDER -> done    Render acknowledged.
 *
 * Other bits are used to keep track of which parts of the pipeline are
 * running synchronously. By default, we can make no assumptions about the
 * synchronicity of PaintControl with a cube's state. As we synchronize
 * or break synchronization with different parts of the pipeline, these
 * bits keep track.
 */

#define _SYS_VBF_DIRTY_RENDER   (1 << 16)   // Still rendering changed VRAM
#define _SYS_VBF_SYNC_ACK       (1 << 17)   // Frame acknowledgment / toggle is synchronous
#define _SYS_VBF_ISR_TRIGGER    (1 << 18)   // Synchronously trigger a paint from radio ISR

#define _SYS_VBF_SYNC           (_SYS_VBF_SYNC_ACK)


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
 * fpContinuous --
 *    When at least this many frames are pending acknowledgment, we
 *    switch to continuous rendering mode. Instead of waiting for a
 *    signal from us, the cubes will just render continuously.
 *
 * fpSingle --
 *    The inverse of fpContinuous. When at least this few frames are
 *    pending, we switch back to one-shot triggered rendering.
 *
 * fpMax --
 *    Maximum number of pending frames to track. If we hit this limit,
 *    Paint() calls will block.
 *
 * fpMin --
 *    Minimum number of pending frames to track. If we go below this
 *    limit, we'll start ignoring acknowledgments.
 */

static const SysTime::Ticks fpsLow = SysTime::hzTicks(4);
static const SysTime::Ticks fpsHigh = SysTime::hzTicks(60);
static const int8_t fpContinuous = 3;
static const int8_t fpSingle = -4;
static const int8_t fpMax = 5;
static const int8_t fpMin = -8;


void PaintControl::waitForPaint()
{
    /*
     * Wait until we're allowed to do another paint. Since our
     * rendering is usually not fully synchronous, this is not nearly
     * as strict as waitForFinish()!
     *
     * Does not require an attached vbuf.
     */

    DEBUG_LOG(("PAINT[%p]: +waitForPaint() pend=%d\n",
        this, pendingFrames));

    for (;;) {
        Atomic::Barrier();
        SysTime::Ticks now = SysTime::ticks();

        // Watchdog expired? Give up waiting.
        if (now > (paintTimestamp + fpsLow))
            break;

        // Wait for minimum frame rate AND for pending renders
        if (now > (paintTimestamp + fpsHigh)
            && pendingFrames <= fpMax)
            break;

        Tasks::work();
        Radio::halt();
    }

    DEBUG_LOG(("PAINT[%p]: -waitForPaint()\n", this));
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

    DEBUG_LOG(("PAINT[%p]: +waitForFinish(), flags=%08x\n", this, vbuf->flags));

    // Disable continuous rendering now, if it was on. This may set _SYS_VBF_DIRTY_VRAM.
    setFlags(vbuf, getFlags(vbuf) & ~_SYS_VF_CONTINUOUS, _SYS_VBF_DIRTY_VRAM);

    SysTime::Ticks deadline = SysTime::ticks() + fpsLow;
    uint32_t mask = _SYS_VBF_DIRTY_RENDER;

    while (mask & Atomic::Load(vbuf->flags)) {

        if (SysTime::ticks() > deadline) {
            // Watchdog expired. Give up waiting, and assume we're fully sync'ed
            DEBUG_LOG(("PAINT[%p]: waitForFinish() -- TIMED OUT\n", this));
            break;
        }

        Tasks::work();
        Radio::halt();
    }

    /*
     * Assume we have no pending frames at this point, either because we
     * waited for them to finish explicitly, or because we waited a while
     * and hit our timeout.
    */

    pendingFrames = 0;
    Atomic::And(vbuf->flags, ~_SYS_VBF_DIRTY_RENDER);

    DEBUG_LOG(("PAINT[%p]: -waitForFinish()\n", this));
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

    DEBUG_LOG(("PAINT[%p]: triggerPaint, flags=%08x\n", this, vbuf->flags));

    int32_t pending = Atomic::Load(pendingFrames);
    int32_t newPending = pending;

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
     * If we're in continuous rendering, count every single
     * Paint invocation for the purposes of loosely matching them with
     * acknowledged frames. This isn't a strict 1:1 mapping, but
     * it's used to close the loop on repaint speed.
     */

    if (needPaint) {
        DEBUG_LOG(("PAINT[%p]: triggerPaint, NEED_PAINT\n", this));
        newPending++;
    }

    if (newPending > 1) {
        // There can't really be more than one frame in-flight at once.
        // This means we're losing ACK synchronization with the cube.
        Atomic::And(vbuf->flags & ~_SYS_VBF_SYNC_ACK);
    }

    /*
     * We turn on continuous rendering only when we're doing a
     * good job at keeping the cube busy continuously, as measured
     * using our pendingFrames counter. We have some hysteresis,
     * so that continuous rendering is only turned off once the
     * cube is clearly pulling ahead of our ability to provide it
     * with frames.
     *
     * If we're using one-shot rendering mode instead of continuous,
     * rendering is triggered by vramFlushed().
     */

    uint8_t flags = getFlags(vbuf);
    if (cube->isAssetLoading()) {
        DEBUG_LOG(("PAINT[%p]: Assets loading, not using continuous mode\n", this));
        flags &= ~_SYS_VF_CONTINUOUS;

    } else if (newPending >= fpContinuous) {
        // Turning on continuous rendering; clear our SYNC flags.
        DEBUG_LOG(("PAINT[%p]: Continuous rendering on (pend=%d)\n", this, pendingFrames));
        flags |= _SYS_VF_CONTINUOUS;
        Atomic::And(vbuf->flags, ~_SYS_VBF_SYNC);

    } else if (newPending <= fpSingle) {
        DEBUG_LOG(("PAINT[%p]: Continuous rendering off (pend=%d)\n", this, pendingFrames));
        flags &= ~_SYS_VF_CONTINUOUS;
    }

    // Atomically apply our changes to pendingFrames.
    Atomic::Add(pendingFrames, newPending - pending);

    // Switch modes, if that's a thing we're doing.
    setFlags(vbuf, flags, 0);
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
    
    if (count) {
        DEBUG_LOG(("PAINT[%p]: ackFrames(%d), ack=%02x\n", this, count,
            cube->getLastFrameACK()));

        pendingFrames -= count;
    }

    _SYSVideoBuffer *vbuf = cube->getVBuf();
    if (vbuf) {
        uint32_t vf = getFlags(vbuf);

        if ((vf & _SYS_VF_CONTINUOUS) == 0 &&
            (vbuf->flags & _SYS_VBF_SYNC_ACK) != 0) {

            // We're running in lockstep with the cube still.



            // Are we doing a deferred TOGGLE from vramFlushed()?
            if (vbuf->flags & _SYS_VBF_DIRTY_TOGGLE) {
                if (cube->getLastFrameACK() & 1)
                    vf &= ~_SYS_VF_TOGGLE;
                else
                    vf |= _SYS_VF_TOGGLE;
                setFlags(vbuf, vf, 0);
                Atomic::And(vbuf->flags, ~_SYS_VBF_DIRTY_TOGGLE);
            }

            // Render is clean
            if (count)
                Atomic::And(vbuf->flags, ~_SYS_VBF_DIRTY_RENDER);
        }
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

    // Wait until paint() has triggered us.
    if ((vbuf->flags & _SYS_VBF_SYNC_PAINT) == 0)
        return;

    uint32_t vf = getFlags(vbuf);
    if (vf & _SYS_VF_CONTINUOUS)
        return;

    if (vbuf->lock)
        return;

    DEBUG_LOG(("PAINT[%p]: vramFlushed(), vf=%02x flags=%08x\n",
        this, vf, vbuf->flags));

    Atomic::Or(vbuf->flags, _SYS_VBF_DIRTY_RENDER);

    /*
     * We can always trigger a render by setting the TOGGLE bit to
     * the inverse of framePrevACK's LSB. If we don't know the ACK data
     * yet, we have to take a guess.
     */
    
    if (cube->hasValidFrameACK() && (cube->getLastFrameACK() & 1) == !!(vf & _SYS_VF_TOGGLE)) {
        // We are in sync with this cube's toggle state. Hooray.
        // We can reliably trigger just by flipping this bit.

        DEBUG_LOG(("PAINT[%p]: TRIGGERING frame toggle (ack=%02x)\n",
            this, cube->getLastFrameACK()));

        vf ^= _SYS_VF_TOGGLE;

    } else {
        // Not in sync! Either we're about to have multiple frames "in flight"
        // simultaneously, or we haven't received a valid ACK yet. We don't
        // want to enable continuous mode, since that would just make the
        // problem worse.

        DEBUG_LOG(("PAINT[%p]: Triggering with toggle FALLBACK\n", this));
        Atomic::Or(vbuf->flags, _SYS_VBF_DIRTY_TOGGLE);
    }

    setFlags(vbuf, vf, 0);
    Atomic::And(vbuf->flags, ~(_SYS_VBF_DIRTY_VRAM | _SYS_VBF_SYNC_PAINT));
}


