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
        Event::setPending(_SYS_CUBE_FOUND, id());
        setConnected();
		
        uint32_t count = Intrinsic::POPCOUNT(CubeSlots::vecConnected);
        LOG(("%u cubes connected\n", count));
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
            paintControl.ackFrames(ack->frame_count - framePrevACK);

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
        int8_t z = -ack->accel[2];

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
