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

#ifdef DEBUG_PAINT
#   define PAINT_LOG(_x)    LOG(_x)
#else
#   define PAINT_LOG(_x)
#endif


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

    PAINT_LOG(("PAINT[%p]: +waitForPaint() pend=%d\n",
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

    PAINT_LOG(("PAINT[%p]: -waitForPaint()\n", this));
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
     *
     * To be 'finished', the following criteria must hold:
     *
     *   - Continuous rendering mode is disabled
     *   - At least one full frame has been rendered since the last VRAM::lock()
     *
     * We track this using the VRAM dirty bits, as documented in abi.h.
     * These bits are polled on a watchdog timer that limits us to waiting
     * at most for our longest frame period.
     */

    PAINT_LOG(("PAINT[%p]: +waitForFinish()\n", this));

    _SYSVideoBuffer *vbuf = cube->getVBuf();
    ASSERT(vbuf);

    // Disable continuous rendering now, if it was on. This may set _SYS_VBF_VRAM.
    setFlags(vbuf, getFlags(vbuf) & ~_SYS_VF_CONTINUOUS);

    /*
     * Flush out all dirty bits, one by one.
     *
     *   VRAM:     Flushed by radio ISR
     *   RENDER:   Flushed by frame ACK
     *
     * We don't have to do any specific work here other than waiting, but
     * if there's a problem with the frame ACK we may get stalled- so there
     * is also a watchdog timer that waits for our longest frame period.
     */

    SysTime::Ticks deadline = SysTime::ticks() + fpsLow;
    while (vbuf->flags & _SYS_VBF_DIRTY_ALL) {
        Atomic::Barrier();

        if (SysTime::ticks() > deadline) {
            // Watchdog expired. Give up waiting, and forcibly reset pendingFrames.
            break;
        }

        Tasks::work();
        Radio::halt();
    }

    // We know rendering is quiet. Take this opportunity to reset.
    reset(cube);

    PAINT_LOG(("PAINT[%p]: -waitForFinish()\n", this));
}

void PaintControl::reset(CubeSlot *cube)
{
    _SYSVideoBuffer *vbuf = cube->getVBuf();
    PAINT_LOG(("PAINT[%p]: reset(), vbuf=%p\n", this, vbuf));

    pendingFrames = 0;

    if (vbuf)
        Atomic::And(vbuf->flags, ~_SYS_VBF_DIRTY_ALL);
}

void PaintControl::triggerPaint(CubeSlot *cube, SysTime::Ticks timestamp,
    bool allowContinuous)
{
    _SYSVideoBuffer *vbuf = cube->getVBuf();

    if (vbuf) {
        PAINT_LOG(("PAINT[%p]: triggerPaint, flags=%08x\n", this, vbuf->flags));

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
         *
         * We don't want to unlock() until we're actually ready to
         * start transmitting data over the radio, so we'll detect new
         * frames by either checking for bits that are already
         * unlocked (in needPaint) or bits that are pending unlock.
         */

        if (needPaint) {
            PAINT_LOG(("PAINT[%p]: triggerPaint, NEED_PAINT\n", this));

            newPending++;

            // Allow the ISR to trigger a paint now.
            Atomic::Or(vbuf->flags, _SYS_VBF_SYNC_PAINT);

            // Make sure we eventually land in vramFlushed: set the
            // last (LSB) bit in cm16.
            Atomic::Or(vbuf->cm16, 1);
        }

        // Atomically apply our changes to pendingFrames.
        Atomic::Add(pendingFrames, newPending - pending);

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
        if (!allowContinuous) {
            PAINT_LOG(("PAINT[%p]: Continuous rendering disallowed\n", this));
            flags &= ~_SYS_VF_CONTINUOUS;
        } else if (newPending >= fpContinuous) {
            PAINT_LOG(("PAINT[%p]: Continuous rendering on (pend=%d)\n", this, pendingFrames));
            flags |= _SYS_VF_CONTINUOUS;
        } else if (newPending <= fpSingle) {
            PAINT_LOG(("PAINT[%p]: Continuous rendering off (pend=%d)\n", this, pendingFrames));
            flags &= ~_SYS_VF_CONTINUOUS;
        }
        setFlags(vbuf, flags);
    }

    /*
     * We must always update paintTimestamp, even if this turned out
     * to be a no-op. An application which makes no changes to VRAM
     * but just calls paint() in a tight loop should iterate at the
     * 'fastPeriod' defined above.
     */
    paintTimestamp = timestamp;
}

void PaintControl::ackFrames(CubeSlot *cube, int32_t count)
{
    /*
     * One or more frames finished rendering on the cube.
     * Use this to update our pendingFrames accumulator.
     *
     * If we are _not_ in continuous rendering mode, we can use this
     * to clear the 'render' dirty bit, indicating that anything
     * which was dirty when this frame started has now been cleaned.
     */

    if (!count)
        return;

    PAINT_LOG(("PAINT[%p]: ackFrames(%d), ack=%02x\n", this, count,
        cube->getLastFrameACK()));

    Atomic::Add(pendingFrames, -(int32_t)count);

    _SYSVideoBuffer *vbuf = cube->getVBuf();
    if (vbuf && (getFlags(vbuf) & _SYS_VF_CONTINUOUS) == 0)
        Atomic::And(vbuf->flags, ~_SYS_VBF_DIRTY_RENDER);
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

    PAINT_LOG(("PAINT[%p]: vramFlushed(), vf=%02x flags=%08x\n",
        this, vf, vbuf->flags));

    Atomic::Or(vbuf->flags, _SYS_VBF_DIRTY_RENDER);

    /*
     * We can always trigger a render by setting the TOGGLE bit to
     * the inverse of framePrevACK's LSB. If we don't know the ACK data
     * yet, we have to take a guess.
     */

    if (cube->hasValidFrameACK()) {
        PAINT_LOG(("PAINT[%p]: TRIGGERING frame toggle (ack=%02x)\n",
            this, cube->getLastFrameACK()));
        vf &= ~_SYS_VF_TOGGLE;
        if (!(cube->getLastFrameACK() & 1))
            vf |= _SYS_VF_TOGGLE;
    } else {
        PAINT_LOG(("PAINT[%p]: Triggering with toggle FALLBACK\n", this));
        vf ^= _SYS_VF_TOGGLE;
    }

    // setFlags will set the DIRTY flag, but we can clear it.
    setFlags(vbuf, vf);
    Atomic::And(vbuf->flags, ~(_SYS_VBF_DIRTY_VRAM | _SYS_VBF_SYNC_PAINT));
}


