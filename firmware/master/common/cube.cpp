/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include <sifteo/machine.h>

#include "cube.h"
#include "vram.h"
#include "accel.h"
#include "flashlayer.h"

#include "neighbors.h"


using namespace Sifteo;

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


#ifndef DISABLE_CUBE


void CubeSlot::loadAssets(_SYSAssetGroup *a)
{
    _SYSAssetGroupCube *ac = assetCube(a);

    if (!ac)
        return;
    if (isAssetGroupLoaded(a))
        return;
    
    // XXX: Pick a base address too!
    ac->progress = 0;
    
    LOG(("FLASH[%d]: Sending asset group %u\n", id(), a->id));
    
    DEBUG_ONLY({
        // In debug builds, we log the asset download time
        assetLoadTimestamp = SysTime::ticks();
    });

    FlashRegion region;
    unsigned entryOffset = a->id * sizeof(AssetIndexEntry);
    // XXX: this is assuming an offset of 0 for the whole asset segment.
    // 'entryOffset' will eventually need to be applied to the offset of the segment itself in flash
    if (!FlashLayer::getRegion(0 + entryOffset, FlashLayer::BLOCK_SIZE, &region)) {
        LOG(("failed to get flash block for asset index entry\n"));
        return;
    }
    AssetIndexEntry entry;
    memcpy(&entry, region.data(), sizeof(AssetIndexEntry));
    FlashLayer::releaseRegion(region);

    unsigned offset = entry.offset;
    if (!FlashLayer::getRegion(offset, sizeof(AssetGroupHeader), &region)) {
        LOG(("failed to get flash block for asset group header\n"));
        return;
    }
    AssetGroupHeader header;
    memcpy(&header, region.data(), sizeof(AssetGroupHeader));
    FlashLayer::releaseRegion(region);

    a->offset = offset + sizeof(AssetGroupHeader);
    a->size = header.dataSize;
    
    // Start by resetting the flash decoder. This must happen before we set 'loadGroup'.
    Atomic::And(CubeSlots::flashResetSent, ~bit());
    Atomic::Or(CubeSlots::flashResetWait, bit());
    
    // Then start streaming asset data for this group
    a->reqCubes |= bit();
    loadGroup = a;
}

bool CubeSlot::radioProduce(PacketTransmission &tx)
{
    /*
     * XXX: Pairing. Try to connect, if we aren't connected. And use a real address.
     *      For now I'm hardcoding the default address, since that's what
     *      the emulator will come up with.
     */

    // always want to run on channel 0x2 in the sim, but allow the channel to
    // be configured when building for hardware
#if defined(MASTER_RF_CHAN) && !defined(SIFTEO_SIMULATOR)
    address.channel = MASTER_RF_CHAN;
#else
    address.channel = 0x2;
#endif
    address.id[0] = id();
    address.id[1] = 0xe7;
    address.id[2] = 0xe7;
    address.id[3] = 0xe7;
    address.id[4] = 0xe7;

    tx.dest = &address;
    tx.packet.len = 0;

    // First priority: Send video buffer updates

    codec.encodeVRAM(tx.packet, vbuf);

    // Second priority: Download assets to flash

    if (CubeSlots::flashResetWait & bit()) {
        /*
         * We need to reset the flash decoder before we can send any data.
         *
         * We can only do this if a reset is needed, hasn't already
         * been sent. Send the reset token, and synchronously reset
         * any flash-related IRQ state.
         *
         * Note the flash reset's dual purpose, of both resetting the
         * cube's flash state machine and triggering the cube to send
         * us an ACK packet with a valid flash byte count. So, we
         * actually end up sending two resets if we haven't yet seen a
         * valid flash ACK from this cube.
         */

        if (CubeSlots::flashResetSent & bit()) {
            // Already sent the reset. Has it timed out?

            if (SysTime::ticks() > flashDeadline) {
                DEBUG_LOG(("FLASH[%d]: Reset timeout\n", id()));
                Atomic::ClearLZ(CubeSlots::flashResetSent, id());
            }

        } else if (codec.flashReset(tx.packet)) {
            // Okay, we sent a reset. Remember to wait for the ACK.

            DEBUG_LOG(("FLASH[%d]: Sending reset token\n", id()));
            Atomic::SetLZ(CubeSlots::flashResetSent, id());
            flashDeadline = SysTime::ticks() + SysTime::msTicks(RTT_DEADLINE_MS);
        }

    } else {
        // Not waiting on a reset. See if we need to send asset data.

        bool done = false;
        _SYSAssetGroup *group = loadGroup;

        if (group && !(group->doneCubes & bit()) &&
            codec.flashSend(tx.packet, group, assetCube(group), done)) {

            if (done) {
                /* Finished asset loading */
                Atomic::SetLZ(group->doneCubes, id());
                Event::setPending(EventBits::ASSETDONE, id());

                DEBUG_ONLY({
                    // In debug builds only, we log the asset download time
                    float seconds = (SysTime::ticks() - assetLoadTimestamp) * (1.0f / SysTime::sTicks(1));
                    LOG(("FLASH[%d]: Finished loading group %p in %.3f seconds\n",
                         id(), group, seconds));
                })
            }
        }
    }

    /*
     * Third priority: Sensor time synchronization
     *
     * XXX: Time syncs are kind of special.  We use them to assign
     *      each cube to a different timeslice of our sensor polling
     *      period, allowing the neighbor sensors to cooperate via
     *      time division multiplexing. The packet itself is a short
     *      (3 byte) and simple packet which simply adjusts the phase
     *      of the cube's sensor timer.
     */

    if (timeSyncState)  {
        timeSyncState--;
    } else if (tx.packet.len == 0) {
        timeSyncState = 1000;
        // cube timer runs at 16Mhz with a prescaler of 12
        const SysTime::Ticks cubeticks = SysTime::ticks() / SysTime::hzTicks(16000000 / 12);

        const uint16_t cubeSyncTime = (cubeticks + (id() * NEIGHBOR_TX_SLOT_TICKS)) % NEIGHBOR_TIMER_PERIOD_TICKS;

        codec.timeSync(tx.packet, cubeSyncTime);
        tx.noAck = true;    // just throw it out there UDP style
        return true;
    }

    // Finalize this packet. Must be last.
    codec.endPacket(tx.packet);

    /*
     * XXX: We don't have to always return true... we can return false if
     *      we have no useful work to do, so long as we still occasionally
     *      return true to request a ping packet at some particular interval.
     */
    return true;
}

void CubeSlot::radioAcknowledge(const PacketBuffer &packet)
{
    if (!connected()) {
        Event::setPending(EventBits::CUBEFOUND, id());
        setConnected();
		
        uint32_t count = Intrinsic::POPCOUNT(CubeSlots::vecConnected);
        LOG(("%u cubes connected\n", count));
        if (count >= CubeSlots::minCubes) {
            Event::resume();
        }
    }
    
    // If we're expecting a stale packet, completely ignore its contents.
    if (CubeSlots::expectStaleACK & bit()) {
        Atomic::ClearLZ(CubeSlots::expectStaleACK, id());
        return;
    }
    
    RF_ACKType *ack = (RF_ACKType *) packet.bytes;

    if (packet.len >= offsetof(RF_ACKType, frame_count) + sizeof ack->frame_count) {
        // This ACK includes a valid frame_count counter

        if (CubeSlots::frameACKValid & bit()) {
            // Some frame(s) finished rendering.

            uint8_t frameACK = ack->frame_count - framePrevACK;
            Atomic::Add(pendingFrames, -(int32_t)frameACK);

        } else {
            Atomic::SetLZ(CubeSlots::frameACKValid, id());
        }

        framePrevACK = ack->frame_count;
    }

    if (packet.len >= offsetof(RF_ACKType, flash_fifo_bytes) + sizeof ack->flash_fifo_bytes) {
        // This ACK includes a valid flash_fifo_bytes counter

        if (CubeSlots::flashACKValid & bit()) {
            // Two valid ACKs in a row, we can count bytes.

            uint8_t loadACK = ack->flash_fifo_bytes - flashPrevACK;
        
            DEBUG_LOG(("FLASH[%d]: Valid ACK for %d bytes (resetWait=%d, resetSent=%d)\n",
                id(), loadACK,
                !!(CubeSlots::flashResetWait & bit()),
                !!(CubeSlots::flashResetSent & bit())));

            if ((CubeSlots::flashResetWait & bit()) && (CubeSlots::flashResetSent & bit())) {
                // We're waiting on a reset
                if (loadACK)
                    Atomic::ClearLZ(CubeSlots::flashResetWait, id());
            } else {
                // Acknowledge FIFO bytes

                /*
                 * XXX: Since we can always lose ACK packets without
                 *      warning, we could theoretically deadlock here,
                 *      where we perpetually wait on an ACK that the
                 *      cube has already sent. Since flash writes are
                 *      somewhat pipelined, in practice we'd actually
                 *      have to lose several ACKs in a row. But it
                 *      could indeed happen. We should have a watchdog
                 *      here, to assume FIFO has been drained (or
                 *      explicitly request another flash ACK) if we've
                 *      been waiting for more than some safe amount of
                 *      time.
                 *
                 *      Alternatively, we could solve this on the cube
                 *      end by having it send a longer-than-strictly-
                 *      necessary ACK packet every so often.
                 */

                codec.flashAckBytes(loadACK);
            }

        } else {
            // Now we've seen one ACK
            Atomic::SetLZ(CubeSlots::flashACKValid, id());
        }

        flashPrevACK = ack->flash_fifo_bytes;
    }

    if (packet.len >= offsetof(RF_ACKType, accel) + sizeof ack->accel) {
        // Has valid accelerometer data. Is it different from our previous state?

        // Translate from radio packet coordinates to SDK coordinates
        int8_t x = -ack->accel[0];
        int8_t y = -ack->accel[1];

        //test for gestures
        AccelState &accel = AccelState::getInstance( id() );
        accel.update(x, y);

        if (x != accelState.x || y != accelState.y) {
            accelState.x = x;
            accelState.y = y;
            Event::setPending(EventBits::ACCELCHANGE, id());
        }
    }

    if (packet.len >= offsetof(RF_ACKType, neighbors) + sizeof ack->neighbors) {
        // Has valid neighbor/flag data

        if (CubeSlots::neighborACKValid & bit()) {
            // Look for valid touches, signified by any edge on the touch toggle bit

            if ((neighbors[0] ^ ack->neighbors[0]) & NB0_FLAG_TOUCH) {
                Event::setPending(EventBits::TOUCH, id());
            }

            Event::setPending(EventBits::NEIGHBOR, id());

        } else {
            Atomic::SetLZ(CubeSlots::neighborACKValid, id());
        }

        // Store the raw state
        neighbors[0] = ack->neighbors[0];
        neighbors[1] = ack->neighbors[1];
        neighbors[2] = ack->neighbors[2];
        neighbors[3] = ack->neighbors[3];
    }
    
    if (packet.len >= offsetof(RF_ACKType, battery_v) + sizeof ack->battery_v) {
        // Has valid battery voltage
        
        rawBatteryV = ack->battery_v;
    }
    
    if (packet.len >= offsetof(RF_ACKType, hwid) + sizeof ack->hwid) {
        // Has valid hardware ID
        
        memcpy(hwid.bytes, ack->hwid, sizeof ack->hwid);
    }
}

void CubeSlot::getRawNeighbors(uint8_t buf[4]) {
    // XXX: Raw neighbor data for testing/demoing only
    buf[0] = neighbors[0];
    buf[1] = neighbors[1];
    buf[2] = neighbors[2];
    buf[3] = neighbors[3];
}

// Are we being touched right now?
bool CubeSlot::isTouching() const {
    // touch state is transmitted in the NB0_FLAG_TOUCH bit
    // of the first neighbor value
    return neighbors[0] & NB0_FLAG_TOUCH;
}

void CubeSlot::radioTimeout()
{
    /* XXX: Disconnect this cube */
    
    if (connected()) {
        Event::setPending(EventBits::CUBELOST, id());
        setDisconnected();
		
        uint32_t count = Intrinsic::POPCOUNT(CubeSlots::vecConnected);
        LOG(("%u cubes connected\n", count));
        if (count < CubeSlots::minCubes) {
            Event::pause();
        }
    }
}

void CubeSlot::waitForPaint()
{
    /*
     * Wait until we're allowed to do another paint. Since our
     * rendering is usually not fully synchronous, this is not nearly
     * as strict as waitForFinish()!
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

        Radio::halt();
    }
}

void CubeSlot::waitForFinish()
{
    /*
     * Wait until all previous rendering has finished, and all of VRAM
     * has been updated over the radio.  Does *not* wait for any
     * minimum frame rate. If no rendering is pending, we return
     * immediately.
     *
     * Continuous rendering is turned off, if it was on.
     */

    uint8_t flags = VRAM::peekb(*vbuf, offsetof(_SYSVideoRAM,flags));
    if (flags & _SYS_VF_CONTINUOUS) {
        /*
         * If we were in continuous rendering mode, pendingFrames isn't
         * exact; it's more of a running accumulator. In that case, we have
         * to turn off continuous rendering mode, flush everything over the
         * radio, then do one Paint(). This is what paintSync() does.
         * So, at this point we just need to reset pendingFrames to zero
         * and turn off continuous mode.
         */
         
        pendingFrames = 0;

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

        Radio::halt();
    }
}

void CubeSlot::triggerPaint(SysTime::Ticks timestamp)
{
    _SYSAssetGroup *group = loadGroup;
        
    if (vbuf) {
        uint8_t flags = VRAM::peekb(*vbuf, offsetof(_SYSVideoRAM, flags));
        int32_t pending = Atomic::Load(pendingFrames);
        int32_t newPending = pending;

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

        if (group && !(group->doneCubes & bit())) {
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
            if (CubeSlots::frameACKValid & bit()) {
                flags &= ~_SYS_VF_TOGGLE;
                if (!(framePrevACK & 1))
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
    }

    /*
     * We must always update paintTimestamp, even if this turned out
     * to be a no-op. An application which makes no changes to VRAM
     * but just calls paint() in a tight loop should iterate at the
     * 'fastPeriod' defined above.
     */
    paintTimestamp = timestamp;
}

#endif // DISABLE_CUBE
