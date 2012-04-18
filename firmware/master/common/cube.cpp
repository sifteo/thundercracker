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
#include "flashlayer.h"
#include "svmdebugpipe.h"
#include "tasks.h"
#include "neighbors.h"
#include "paintcontrol.h"
#include "cubeslots.h"

// Simulator headers, for simAssetLoaderBypass.
#ifdef SIFTEO_SIMULATOR
#   include "system_mc.h"
#   include "cube_hardware.h"
#   include "lsdec.h"
#endif


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
            LoadstreamDecoder lsdec(simCube->flashStorage);
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
    Tasks::setPending(Tasks::AssetLoader);
}

void CubeSlot::requestFlashReset()
{
    Atomic::And(CubeSlots::flashResetSent, ~bit());
    Atomic::Or(CubeSlots::flashResetWait, bit());
}

const RadioAddress *CubeSlot::getRadioAddress()
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

    return &address;
}

bool CubeSlot::radioProduce(PacketTransmission &tx)
{
    tx.dest = getRadioAddress();
    tx.packet.len = 0;

    // First priority: Send video buffer updates

    if (codec.encodeVRAM(tx.packet, vbuf))
        paintControl.vramFlushed(this);

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
                    Event::setPending(_SYS_CUBE_ASSETDONE, id());

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

    // Debugging: Scrub the VRAM if we have no video data in-flight
    DEBUG_ONLY({
        _SYSVideoBuffer *vb = vbuf;
        if (hasContent)
            consecutiveEmptyPackets = 0;
        else if (++consecutiveEmptyPackets == 3 && vbuf
            && vbuf->lock == 0 && vbuf->cm16 == 0)
            SystemMC::checkQuiescentVRAM(this);
    });

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
        Event::setPending(_SYS_CUBE_FOUND, id());
        setConnected();

        LOG(("%u cubes connected\n",
            Intrinsic::POPCOUNT(CubeSlots::vecConnected)));
    }
    
    // If we're expecting a stale packet, completely ignore its contents.
    if (CubeSlots::expectStaleACK & bit()) {
        Atomic::ClearLZ(CubeSlots::expectStaleACK, id());
        return;
    }

    RF_ACKType *ack = (RF_ACKType *) packet.bytes;

    if (packet.len >= offsetof(RF_ACKType, frame_count) + sizeof ack->frame_count) {
        // This ACK includes a valid frame_count counter

        uint8_t delta = ack->frame_count - framePrevACK;
        framePrevACK = ack->frame_count;

        if ((CubeSlots::frameACKValid & bit()) == 0) {
            Atomic::SetLZ(CubeSlots::frameACKValid, id());
        } else if (delta) {
            // Some frame(s) finished rendering.
            paintControl.ackFrames(this, delta);
        }
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

        int8_t x = ack->accel[0];
        int8_t y = ack->accel[1];
        int8_t z = ack->accel[2];

        // Test for gestures
        AccelState &accel = AccelState::getInstance( id() );
        accel.update(x, y);

        if (x != accelState.x || y != accelState.y || z != accelState.z) {
            accelState.x = x;
            accelState.y = y;
            accelState.z = z;
            Event::setPending(_SYS_CUBE_ACCELCHANGE, id());
        }
    }

    if (packet.len >= offsetof(RF_ACKType, neighbors) + sizeof ack->neighbors) {
        // Has valid neighbor/flag data

        if (CubeSlots::neighborACKValid & bit()) {
            // Look for valid touches, signified by any edge on the touch toggle bit

            if ((neighbors[0] ^ ack->neighbors[0]) & NB0_FLAG_TOUCH) {
                Event::setPending(_SYS_CUBE_TOUCH, id());
            }

            // Trigger a rescan of all neighbors, during event dispatch
            Event::setPending(_SYS_NEIGHBOR_ADD, id());

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
        
        rawBatteryV = ack->battery_v[0] | (ack->battery_v[1] << 8);
    }
    
    if (packet.len >= offsetof(RF_ACKType, hwid) + sizeof ack->hwid) {
        // Has valid hardware ID

        STATIC_ASSERT(sizeof hwid == sizeof ack->hwid);
        memcpy(hwid, ack->hwid, sizeof ack->hwid);
        Atomic::SetLZ(CubeSlots::hwidValid, id());
    }
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
        Event::setPending(_SYS_CUBE_LOST, id());
        setDisconnected();
		
        uint32_t count = Intrinsic::POPCOUNT(CubeSlots::vecConnected);
        LOG(("%u cubes connected\n", count));
    }
}

uint64_t CubeSlot::getHWID()
{
    /*
     * Return this cube's Hardware ID. If we don't know the ID yet, this
     * blocks until the ID can be retrieved over the radio. (This happens
     * as a side-effect of completing a Flash Reset.)
     */

    if (!(CubeSlots::hwidValid & bit())) {
        if (!enabled() || !connected()) {
            // Cube disappeared! Cancel.
            return _SYS_INVALID_HWID;
        }

        // If no assets are loading / have loaded, send our own reset.
        // (We don't want to stomp on an ongoing asset download!)
        if (!isAssetLoading())
            requestFlashReset();

        do {
            Tasks::work();
            Radio::halt();
        } while (!(CubeSlots::hwidValid & bit()));
    }
    
    uint64_t result = 0;
    memcpy(&result, hwid, sizeof hwid);
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
     * cubes increases.
     */

    // 5-bit lookup table for bit reversal
    static const uint8_t rev5[] = {
        0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c,
        0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
        0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d,
        0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f,
    };

    STATIC_ASSERT(arraysize(rev5) == _SYS_NUM_CUBE_SLOTS);
    const unsigned slotWidth = timerPeriod / _SYS_NUM_CUBE_SLOTS;
    unsigned slotID = rev5[id()];

    return (cubeTicks + slotID * slotWidth) & timerMask;
}
