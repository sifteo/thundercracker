/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>

#include "machine.h"
#include "cube.h"
#include "vram.h"
#include "accel.h"
#include "event.h"
#include "flash_blockcache.h"
#include "svmdebugpipe.h"
#include "tasks.h"
#include "neighborslot.h"
#include "paintcontrol.h"
#include "cubeslots.h"
#include "assetslot.h"

// Simulator headers, for simAssetLoaderBypass.
#ifdef SIFTEO_SIMULATOR
#   include "system_mc.h"
#   include "cube_hardware.h"
#   include "lsdec.h"
#endif


void CubeSlot::connect(SysLFS::Key cubeRecord, const RadioAddress &addr, const RF_ACKType &fullACK)
{
    _SYSCubeIDVector cv = bit();

    // Reset state
    NeighborSlot::resetSlots(cv);
    setVideoBuffer(0);
    Atomic::And(CubeSlots::flashResetWait, ~cv);
    Atomic::And(CubeSlots::flashResetSent, ~cv);
    Atomic::And(CubeSlots::flashAddrPending, ~cv);
    Atomic::And(CubeSlots::sendShutdown, ~cv);
    Atomic::And(CubeSlots::sendStipple, ~cv);
    Atomic::And(CubeSlots::vramPaused, ~cv);
    Atomic::And(CubeSlots::touch, ~cv);

    // Store new identity
    lastACK = fullACK;
    address = addr;
    this->cubeRecord = cubeRecord;

    LOG(("CUBE[%d]: Connected to system. "
        "record=%02x addr=%02x/%02x%02x%02x%02x%02x hwid=%016"PRIx64"\n",
        id(), cubeRecord, addr.channel, addr.id[0], addr.id[1], addr.id[2], addr.id[3],
        addr.id[4], getHWID()));

    // The cube is now connected. At this instant we may start sending packets to it.
    Atomic::Or(CubeSlots::sysConnected, cv);
    CubeSlots::pairConnected.atomicMark(cubeRecord - SysLFS::kCubeBase);

    // Propagate this connection to userspace
    Event::setCubePending(Event::PID_CONNECTION, id());
}

void CubeSlot::disconnect()
{
    LOG(("CUBE[%d]: Disconnected from system\n", id()));
    _SYSCubeIDVector cv = bit();

    // Make sure we dispatch a user disconnect event, even if another cube
    // or the same cube reconnects in the same slot before we can dispatch the event.
    Atomic::Or(CubeSlots::disconnectFlag, cv);

    // Disconnect it from the system; the user will follow when we dispatch the event.
    Atomic::And(CubeSlots::sysConnected, ~cv);

    // Begin trying to reconnect
    CubeSlots::pairConnected.atomicClear(cubeRecord - SysLFS::kCubeBase);

    // Propagate this disconnection to userspace
    Event::setCubePending(Event::PID_CONNECTION, id());

    setVideoBuffer(0);
    NeighborSlot::resetSlots(cv);
    NeighborSlot::resetPairs(cv);
}

void CubeSlot::userConnect()
{
    Atomic::Or(CubeSlots::userConnected, bit());
    setVideoBuffer(0);
    VirtAssetSlots::rebindCube(id());
}

void CubeSlot::userDisconnect()
{
    Atomic::And(CubeSlots::userConnected, ~bit());
}

bool CubeSlot::isTouching() const
{
    /*
     * For pulse-stretching, we need to register a touch if either
     * the current ACK response indicates a touch, OR if we're holding
     * onto a touch in CubeSlots::touch.
     */

    return (bit() & CubeSlots::touch) || (lastACK.neighbors[0] & NB0_FLAG_TOUCH);
}

void CubeSlot::clearTouchEvent() const
{
    /*
     * Clear the pulse-stretched version of our 'touch' state, once we've
     * sent a touch event to userspace. Note that this may require sending
     * another touch event so that userspace can see the cleared state, in
     * case both a press and release happened before userspace got to see
     * the press.
     */

    ASSERT(CubeSlots::touch & bit());
    Atomic::ClearLZ(CubeSlots::touch, id());

    if (!isTouching()) {
        Event::setCubePending(Event::PID_CUBE_TOUCH, id());
    }
}

void CubeSlot::startAssetLoad(SvmMemory::VirtAddr groupVA, uint16_t baseAddr)
{
    /*
     * Trigger the beginning of an asset group installation for this cube.
     * There must be a SYSAssetLoader currently set.
     */

    // Translate and verify addresses
    SvmMemory::PhysAddr groupPA;
    if (!SvmMemory::mapRAM(groupVA, sizeof(_SYSAssetGroup), groupPA))
        return;
    _SYSAssetGroup *G = reinterpret_cast<_SYSAssetGroup*>(groupPA);
    _SYSAssetLoader *L = CubeSlots::assetLoader;
    if (!L) return;
    _SYSAssetLoaderCube *LC = assetLoaderCube(L);
    if (!LC) return;
    _SYSAssetGroupCube *GC = assetGroupCube(G);
    if (!GC) return;

    // Read (cached) asset group header. Must be valid.
    const _SYSAssetGroupHeader *headerVA =
        reinterpret_cast<const _SYSAssetGroupHeader*>(G->pHdr);
    _SYSAssetGroupHeader header;
    if (!SvmMemory::copyROData(header, headerVA))
        return;

    // Because we're storing this in a 32-bit struct field, squash groupVA
    SvmMemory::squashPhysicalAddr(groupVA);

    // Initialize state
    Atomic::ClearLZ(L->complete, id());
    GC->baseAddr = baseAddr;
    LC->pAssetGroup = groupVA;
    LC->progress = 0;
    LC->dataSize = header.dataSize;
    LC->reserved = 0;
    LC->head = 0;
    LC->tail = 0;

    #ifdef SIFTEO_SIMULATOR
    if (CubeSlots::simAssetLoaderBypass) {
        /*
         * Asset loader bypass mode: Instead of actually sending this
         * loadstream over the radio, instantaneously decompress it into
         * the cube's flash memory.
         */

        // Use our reference implementation of the Loadstream decoder
        Cube::Hardware *simCube = SystemMC::getCubeForSlot(this);
        if (simCube) {
            FlashStorage::CubeRecord *storage = simCube->flash.getStorage();
            LoadstreamDecoder lsdec(storage->ext, sizeof storage->ext);

            lsdec.setAddress(baseAddr << 7);
            lsdec.handleSVM(G->pHdr + sizeof header, header.dataSize);

            LOG(("FLASH[%d]: Installed asset group %s at base address "
                "0x%08x (loader bypassed)\n",
                id(), SvmDebugPipe::formatAddress(G->pHdr).c_str(), baseAddr));

            // Mark this as done already.
            LC->progress = header.dataSize;
            Atomic::SetLZ(L->complete, id());

            return;
        }
    }
    #endif

    LOG(("FLASH[%d]: Sending asset group %s, at base address 0x%08x\n",
        id(), SvmDebugPipe::formatAddress(G->pHdr).c_str(), baseAddr));

    DEBUG_ONLY({
        // In debug builds, we log the asset download time
        assetLoadTimestamp = SysTime::ticks();
    });

    // Start by resetting the flash decoder.
    requestFlashReset();
    Atomic::SetLZ(CubeSlots::flashAddrPending, id());

    // Only _after_ triggering the reset, start the actual download
    // by marking cubeVec as valid.
    Atomic::SetLZ(L->cubeVec, id());

    // Start filling our asset data FIFOs.
    Tasks::trigger(Tasks::AssetLoader);
}

void CubeSlot::requestFlashReset()
{
    Atomic::And(CubeSlots::flashResetSent, ~bit());
    Atomic::Or(CubeSlots::flashResetWait, bit());
}

bool CubeSlot::radioProduce(PacketTransmission &tx)
{
    _SYSCubeIDVector cv = bit();
    tx.dest = getRadioAddress();
    tx.packet.len = 0;

    /* 
     * First priority: Send VRAM data.
     *
     * Normally this comes from a general-purpose VideoBuffer, though we
     * do have some special-purpose ways to send VRAM data without a
     * VideoBuffer.
     */

    if (UNLIKELY(CubeSlots::sendShutdown & cv)) {
        /*
         * Tell this cube to power off, by sending it to _SYS_VM_SLEEP.
         * We leave this bit set; if the cube doesn't go to sleep right
         * away, keep telling it to. Once we succeed, the cube will
         * disconnect.
         */

        codec.encodePoke(tx.packet, offsetof(_SYSVideoRAM, mode)/2,
            _SYS_VM_SLEEP | (_SYS_VF_CONTINUOUS << 8));

        ASSERT(!tx.packet.isFull());

    } else if (UNLIKELY(CubeSlots::sendStipple & cv)) {
        /* 
         * Show that this cube is paused, by having it draw a stipple pattern.
         *
         * Note, we expect this cube to have finished rendering already,
         * or it may end up drawing a garbage frame behind the stipple!
         *
         * This does not require us to have a vbuf attached. But if we
         * do, we'll use it to be a good citizen and avoid using continuous
         * mode or causing an unintentional rotation change.
         */

        _SYSVideoBuffer *localVBuf = vbuf;
        unsigned modeFlagsWord = _SYS_VM_STAMP;
        if (localVBuf) {
            uint8_t flags = localVBuf->vram.flags ^ _SYS_VF_TOGGLE;
            localVBuf->flags = flags;
            modeFlagsWord |= flags << 8;
        } else {
            modeFlagsWord |= _SYS_VF_CONTINUOUS << 8;
        }

        codec.encodePoke(tx.packet, offsetof(_SYSVideoRAM, fb)/2,          0x0220);
        codec.encodePoke(tx.packet, offsetof(_SYSVideoRAM, colormap[2])/2, 0x0000);
        codec.encodePoke(tx.packet, offsetof(_SYSVideoRAM, stamp_pitch)/2, 0x0201);
        codec.encodePoke(tx.packet, offsetof(_SYSVideoRAM, stamp_x)/2,     0x8000);
        codec.encodePoke(tx.packet, offsetof(_SYSVideoRAM, stamp_key)/2,   0x0000);
        codec.encodePoke(tx.packet, offsetof(_SYSVideoRAM, first_line)/2,  0x8000);
        codec.encodePoke(tx.packet, offsetof(_SYSVideoRAM, mode)/2,        modeFlagsWord);

        ASSERT(!tx.packet.isFull());
        Atomic::And(CubeSlots::sendStipple, ~cv);

    } else if (LIKELY(0 == (CubeSlots::vramPaused & cv))) {
        // Normal updates from VideoBuffer

        if (codec.encodeVRAM(tx.packet, vbuf))
            if (paintControl.vramFlushed(this))
                codec.encodeVRAM(tx.packet, vbuf);
    }

    /*
     * Second priority: Download assets to flash
     */

    if (CubeSlots::flashResetWait & cv) {
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

        if (CubeSlots::flashResetSent & cv) {
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
        // Since we can't read external flash pages in our ISR, we're
        // restricted to accessing user RAM only. So, we send data from
        // a small user-ram buffer, and use a Task to refill that buffer.

        _SYSAssetLoader *L = CubeSlots::assetLoader;
        if (isAssetLoading(L)) {
            _SYSAssetLoaderCube *LC = assetLoaderCube(L);
            if (LC) {
                bool done = false;
                bool escape = codec.flashSend(tx.packet, LC, id(), done);

                if (done) {
                    /* Finished sending the group, and the cube finished writing it. */
                    Atomic::SetLZ(L->complete, id());
                    Event::setCubePending(Event::PID_CUBE_ASSETDONE, id());

                    DEBUG_ONLY({
                        // In debug builds only, we log the asset download time
                        float seconds = (SysTime::ticks() - assetLoadTimestamp) * (1.0f / SysTime::sTicks(1));
                        LOG(("FLASH[%d]: Finished loading in %.3f seconds\n", id(), seconds));
                    });
                }

                // We can't put anything else in this packet if an escape was written
                if (escape)
                    return true;
            }
        }
    }

    /*
     * Third priority: Sensor time synchronization
     *
     * Time syncs are kind of special.  We use them to assign
     * each cube to a different timeslice of our sensor polling
     * period, allowing the neighbor sensors to cooperate via
     * time division multiplexing. The packet itself is a short
     * (3 byte) and simple packet which simply adjusts the phase
     * of the cube's sensor timer.
     */

    if (timeSyncState)  {
        timeSyncState--;
    } else if (tx.packet.len == 0) {
        timeSyncState = 1000;
        codec.timeSync(tx.packet, calculateTimeSync());
        tx.noAck = true;    // just throw it out there UDP style
        return true;
    }

    // Finalize this packet. Must be last.
    bool hasContent = codec.endPacket(tx.packet);

    /*
     * XXX: We don't have to always return true... we can return false if
     *      we have no useful work to do, so long as we still occasionally
     *      return true to request a ping packet at some particular interval.
     */
    return true;
}

void CubeSlot::radioAcknowledge(const PacketBuffer &packet)
{
    RF_ACKType *ack = (RF_ACKType *) packet.bytes;

    // ACKs are always at least one byte.
    if (packet.len < 1) {
        ASSERT(0 && "Empty ACK packet. Radio bug?");
        return;
    }

    // If this is a query response, it doesn't follow the usual ACK format.
    // (Queries include an ID, so this isn't subject to the 'stale ACK' test below)
    if (ack->frame_count & QUERY_ACK_BIT) {
        queryResponse(packet);
        return;
    }

    // All ACKs have a header byte with frame rate control info
    {
        uint8_t delta = ack->frame_count - lastACK.frame_count;
        delta &= FRAME_ACK_COUNT;
        if (delta)
            paintControl.ackFrames(this, delta);
    }

    if (packet.len >= offsetof(RF_ACKType, flash_fifo_bytes) + sizeof ack->flash_fifo_bytes) {
        // This ACK includes a valid flash_fifo_bytes counter

        uint8_t loadACK = ack->flash_fifo_bytes - lastACK.flash_fifo_bytes;

        DEBUG_LOG(("FLASH[%d]: Valid ACK for %d bytes (resetWait=%d, resetSent=%d)\n",
            id(), loadACK,
            !!(CubeSlots::flashResetWait & cv),
            !!(CubeSlots::flashResetSent & cv)));

        /*
         * Acknowledge FIFO bytes
         *
         * Note that these ACKs may get lost; CubeCodec will explicitly request
         * a resend if it's out of buffer space! (Normally dropped ACKs aren't
         * an issue, since we'll have other ACKs in the pipeline. But if we hit
         * a pipeline bubble and/or multiple ACKs drop in a row, we need to
         * intervene)
         */
        codec.flashAckBytes(loadACK);
    }

    if (packet.len >= offsetof(RF_ACKType, accel) + sizeof ack->accel) {
        // Has valid accelerometer data. Is it different from our previous state?

        // Test for gestures
        AccelState &accel = AccelState::getInstance( id() );
        accel.update(ack->accel[0], ack->accel[1]);

        if (memcmp(lastACK.accel, ack->accel, sizeof lastACK.accel))
            Event::setCubePending(Event::PID_CUBE_ACCELCHANGE, id());
    }

    if (packet.len >= offsetof(RF_ACKType, neighbors) + sizeof ack->neighbors) {
        // Has valid neighbor/flag data

        /*
         * Look for valid touch up/down events, signified by any edge on the touch toggle bit.
         * We maintain a pulse-stretched version in CubeSlots::touch, which we can set here but
         * which is never cleared until userspace has a chance to see the event.
         *
         * We never clear CubeSlots::touch here. That's done by clearTouchEvent(), after
         * we finish dispatching events to userspace.
         */
        if ((lastACK.neighbors[0] ^ ack->neighbors[0]) & NB0_FLAG_TOUCH) {
            if (ack->neighbors[0] & NB0_FLAG_TOUCH) {
                Atomic::SetLZ(CubeSlots::touch, id());
            }
            Event::setCubePending(Event::PID_CUBE_TOUCH, id());
        }

        // Is this a flash reset ACK?
        if ((lastACK.neighbors[1] ^ ack->neighbors[1]) & NB1_FLAG_FLS_RESET) {
            Atomic::ClearLZ(CubeSlots::flashResetWait, id());
        }

        // Trigger a rescan of all neighbors, during event dispatch
        Event::setCubePending(Event::PID_NEIGHBORS, id());
    }

    if (packet.len >= offsetof(RF_ACKType, battery_v) + sizeof ack->battery_v) {
        // Packet has a valid battery voltage. Dispatch an event, if it's changed.

        if (lastACK.battery_v != ack->battery_v)
            Event::setCubePending(Event::PID_CUBE_BATTERY, id());
    }

    if (packet.len >= offsetof(RF_ACKType, hwid) + sizeof ack->hwid) {
        // Has valid hardware ID. We already know the cube's HWID from when we
        // first connected it... but just out of paranoia, check whether it's changed
        // and disconnect the cube if so.

        if (memcmp(lastACK.hwid, ack->hwid, sizeof ack->hwid))
            disconnect();
    }

    // Store the mutable parts of the ACK packet (Prior to the HWID)
    memcpy(&lastACK, ack, MIN(offsetof(RF_ACKType, hwid), packet.len));
}

void CubeSlot::radioTimeout()
{
    disconnect();
}

uint64_t CubeSlot::getHWID()
{
    uint64_t result = 0;
    memcpy(&result, lastACK.hwid, sizeof lastACK.hwid);
    return result;
}

uint16_t CubeSlot::calculateTimeSync()
{
    /*
     * Calculate the phase value to, at this very moment, deliver to the
     * cube in order to settle it into the proper neighbor timeslot.
     */

    /*
     * This returns a raw timer reload value. The timer runs off a 16 MHz
     * clock, with a divide-by-12 prescaler. The counter rolls over at 13 bits.
     */
    const SysTime::Ticks cubeTicks = SysTime::ticks() / SysTime::hzTicks(16000000 / 12);
    const unsigned timerPeriod = 0x2000;
    const unsigned timerMask   = 0x1FFF;

    /*
     * We nominally want to divide this period totally evenly by
     * _SYS_NUM_CUBE_SLOTS, to give each cube the same size allocation.
     * However, any padding we can provide around the slots can help us
     * be more robust in the face of failures, plus they can help us
     * debug hardware issues by occasionally choosing a slower baud rate
     * for the neighbor packets.
     *
     * An easy way to do this is to reverse the bits in our cube ID number,
     * then scale the resulting number by the nominal size of a slot. This
     * way, the slots appear to subdivide every time log2(N) of the number of
     * cubes increases. Note that our slot width is 1/32nd of the period, as
     * that's the smallest power of two >= _SYS_NUM_CUBE_SLOTS.
     */

    // 5-bit lookup table for bit reversal
    static const uint8_t rev5[] = {
        0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c,  // 8
        0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,  // 16
        0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d,  // 24
     /* 0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f,     32 (unused) */
    };

    STATIC_ASSERT(_SYS_NUM_CUBE_SLOTS <= 32);
    STATIC_ASSERT(arraysize(rev5) == _SYS_NUM_CUBE_SLOTS);
    const unsigned slotWidth = timerPeriod / 32;
    unsigned slotID = rev5[id()];

    return (cubeTicks + slotID * slotWidth) & timerMask;
}

void CubeSlot::queryResponse(const PacketBuffer &packet)
{
    /// XXX implement me
}
