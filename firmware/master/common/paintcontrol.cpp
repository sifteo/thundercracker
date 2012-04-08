/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "paintcontrol.h"
#include "tasks.h"
#include "radio.h"
#include "vram.h"
#include "cube.h"

#ifdef DEBUG_PAINT
#   define PAINT_LOG(_x)    LOG(_x)
#else
#   define PAINT_LOG(_x)
#endif


/*
 * Frame rate control parameters
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
}

void PaintControl::waitForFinish(_SYSVideoBuffer *vbuf)
{
    /*
     * Wait until all previous rendering has finished, and all of VRAM
     * has been updated over the radio.  Does *not* wait for any
     * minimum frame rate. If no rendering is pending, we return
     * immediately.
     *
     * To be 'finished', the following criteria must hold:
     *
     *   - Continuous rendering mode is disabled
     *   - At least one full frame has been rendered since the last
     *     unlock() which committed a nonzero set of changes to VRAM.
     */

    PAINT_LOG(("PAINT[%d]: waitForFinish, vbuf=%p \n", id(), vbuf));

    if (!vbuf)
        return;

    uint8_t flags = VRAM::peekb(*vbuf, offsetof(_SYSVideoRAM,flags));

    PAINT_LOG(("PAINT[%d]: waitForFinish, flags=%02x pending=%d\n",
        id(), flags, pendingFrames));

    /*
     * Disable continuous rendering now, if it was on.
     */
    if (flags & _SYS_VF_CONTINUOUS) {
        VRAM::pokeb(*vbuf, offsetof(_SYSVideoRAM, flags), flags & ~_SYS_VF_CONTINUOUS);
        VRAM::unlock(*vbuf);
    }

    for (;;) {
        Atomic::Barrier();
        SysTime::Ticks now = SysTime::ticks();

        if (now > (paintTimestamp + fpsLow)) {
            // Watchdog expired. Give up waiting, and forcibly reset pendingFrames.
            pendingFrames = 0;
            break;
        }

        if (pendingFrames <= 0 && (vbuf == 0 || vbuf->cm32 == 0)) {
            // No pending renders or VRAM updates
            break;
        }

        Tasks::work();
        Radio::halt();
    }

    PAINT_LOG(("PAINT[%d]: waitForFinish done, flags=%02x pending=%d\n",
        id(), flags, pendingFrames));
}

void PaintControl::triggerPaint(CubeSlot *cube, SysTime::Ticks timestamp)
{
    _SYSVideoBuffer *vbuf = cube->getVBuf();

    if (vbuf) {
        uint8_t flags = VRAM::peekb(*vbuf, offsetof(_SYSVideoRAM, flags));
        int32_t pending = Atomic::Load(pendingFrames);
        int32_t newPending = pending;

        PAINT_LOG(("PAINT[%d]: triggerPaint, flags=%02x pending=%d newPend=%d"
            " needPaint=%x cm32next=%x cm32=%x\n",
            id(), flags, pending, newPending, vbuf->needPaint, vbuf->cm32next,
            vbuf->cm32));

        /*
         * Keep pendingFrames above the lower limit. We make this
         * adjustment lazily, rather than doing it from inside the
         * ISR.
         */
        if (pending < fpMin)
            newPending = fpMin;

        /*
         * Count all requested paint operations, so that we can
         * loosely match them with acknowledged frames. This isn't a
         * strict 1:1 mapping, but it's used to close the loop on
         * repaint speed.
         *
         * We don't want to unlock() until we're actually ready to
         * start transmitting data over the radio, so we'll detect new
         * frames by either checking for bits that are already
         * unlocked (in needPaint) or bits that are pending unlock.
         */
        uint32_t needPaint = vbuf->needPaint | vbuf->cm32next;
        if (needPaint)
            newPending++;

        /*
         * We turn on continuous rendering only when we're doing a
         * good job at keeping the cube busy continuously, as measured
         * using our pendingFrames counter. We have some hysteresis,
         * so that continuous rendering is only turned off once the
         * cube is clearly pulling ahead of our ability to provide it
         * with frames.
         *
         * We only allow continuous rendering when we aren't downloading
         * assets to this cube. Continuous rendering makes flash downloading
         * extremely slow- flash is strictly lower priority than graphics
         * on the cube, but continuous rendering asks the cube to render
         * graphics as fast as possible.
         */

        if (cube->isAssetLoading()) {
            flags &= ~_SYS_VF_CONTINUOUS;
        } else {
            if (newPending >= fpContinuous)
                flags |= _SYS_VF_CONTINUOUS;
            if (newPending <= fpSingle)
                flags &= ~_SYS_VF_CONTINUOUS;
        }
                
        /*
         * If we're not using continuous mode, each frame is triggered
         * explicitly by toggling a bit in the flags register.
         *
         * We can always trigger a render by setting the TOGGLE bit to
         * the inverse of framePrevACK's LSB. If we don't know the ACK data
         * yet, we have to punt and set continuous mode for now.
         */
        if (!(flags & _SYS_VF_CONTINUOUS) && needPaint) {
            if (cube->hasValidFrameACK()) {
                flags &= ~_SYS_VF_TOGGLE;
                if (!(cube->getLastFrameACK() & 1))
                    flags |= _SYS_VF_TOGGLE;
            } else {
                flags |= _SYS_VF_CONTINUOUS;
            }
        }

        /*
         * Atomically apply our changes to pendingFrames.
         */
        Atomic::Add(pendingFrames, newPending - pending);

        /*
         * Now we're ready to set the ISR loose on transmitting this frame over the radio.
         */
        VRAM::pokeb(*vbuf, offsetof(_SYSVideoRAM, flags), flags);
        VRAM::unlock(*vbuf);
        vbuf->needPaint = 0;

        PAINT_LOG(("PAINT[%d]: triggerPaint, pendingFrames=%d (%d), flags=%x\n",
            id(), pendingFrames, newPending - pending, flags));
    }

    /*
     * We must always update paintTimestamp, even if this turned out
     * to be a no-op. An application which makes no changes to VRAM
     * but just calls paint() in a tight loop should iterate at the
     * 'fastPeriod' defined above.
     */
    paintTimestamp = timestamp;
}
