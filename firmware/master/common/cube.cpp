/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>

#include "machine.h"
#include "cube.h"
#include "vram.h"
#include "event.h"
#include "flash_blockcache.h"
#include "svmdebugpipe.h"
#include "tasks.h"
#include "neighborslot.h"
#include "paintcontrol.h"
#include "cubeslots.h"
#include "assetslot.h"
#include "assetloader.h"
#include "cubeconnector.h"
#include "idletimeout.h"
#include "prng.h"
#include "radioaddrfactory.h"
#include "batterylevel.h"


void CubeSlot::connect(SysLFS::Key cubeRecord, const RadioAddress &addr, const RF_ACKType &fullACK)
{
    _SYSCubeIDVector cv = bit();

    // Reset state
    NeighborSlot::resetSlots(cv);
    setVideoBuffer(0);
    setMotionBuffer(0);
    Atomic::And(CubeSlots::sendShutdown, ~cv);
    Atomic::And(CubeSlots::sendStipple, ~cv);
    Atomic::And(CubeSlots::vramPaused, ~cv);
    Atomic::And(CubeSlots::touch, ~cv);
    Atomic::And(CubeSlots::waitingOnCubes, ~cv);
    Atomic::And(CubeSlots::pendingHop, ~cv);
    pendingChannel = INVALID_CHANNEL;
    ackOptional = false;
    napDeadline = 0;

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

    // if this connection means we're full, don't bother trying to connect anybody else
    if (!CubeSlots::connectionSlotsAvailable())
        CubeConnector::disableReconnect();

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

    // if this has freed up a slot, ensure reconnect is enabled
    if (CubeSlots::connectionSlotsAvailable())
        CubeConnector::enableReconnect();

    // Begin trying to reconnect
    CubeSlots::pairConnected.atomicClear(cubeRecord - SysLFS::kCubeBase);

    // Propagate this disconnection to userspace
    Event::setCubePending(Event::PID_CONNECTION, id());

    setVideoBuffer(0);
    setMotionBuffer(0);

    NeighborSlot::instances[id()].resetPairs();
}

void CubeSlot::userConnect()
{
    Atomic::Or(CubeSlots::userConnected, bit());
    setVideoBuffer(0);
    VirtAssetSlots::rebindCube(id());
    AssetLoader::cubeConnect(id());
}

void CubeSlot::userDisconnect()
{
    Atomic::And(CubeSlots::userConnected, ~bit());
    AssetLoader::cubeDisconnect(id());
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

void CubeSlot::setVideoBuffer(_SYSVideoBuffer *v)
{
    if (v) {
        // Update this VideoBuffer's flash bank, if necessary
        VRAMFlags vf(v);
        vf.setTo(_SYS_VF_A21, VirtAssetSlots::getCubeBank(id()));
        vf.apply(v);
    }

    vbuf = v;
}

bool CubeSlot::radioProduce(PacketTransmission &tx, SysTime::Ticks now)
{
    // If the cube is asleep, yield our transmit slot
    if (napDeadline > now)
        return false;

    bool idle = true;
    _SYSCubeIDVector cv = bit();
    tx.dest = getRadioAddress();
    tx.packet.len = 0;

    /*
     * First priority: radio hops
     *
     * If we detect that communications are getting flaky (according
     * to our software retry count) we may opt to try and move a cube
     * to a new (and hopefully better) channel.
     *
     * Hops are asynchronous. They need to go at the end of the packet since
     * they're a type of escape, which means that we send a short packet
     * whenever we try to hop. The current motivation to put them at
     * highest priority is that maintaining a (slightly less optimized)
     * link to the cube is more important than encoding packets more efficiently
     * that won't actually get delivered.
     *
     * Fortunately these tend to happen only once every several minutes
     * during normal operation.
     *
     * Note that we do want to retry, but if the hop succeeds and we drop
     * an ACK, that would appear to be a timeout even though it doesn't
     * represent a problem. (If we really did drop all transmit attempts,
     * the cube will not have hopped and we'll disconnect it.)
     *
     * We need to remember not to worry about retries then, by setting our
     * "ackOptional" flag.
     */

    if (UNLIKELY(CubeSlots::pendingHop & cv)) {
        unsigned ch = RadioManager::suggestChannel();
        if (codec.escChannelHop(tx.packet, ch)) {
            pendingChannel = ch;
            Atomic::And(CubeSlots::pendingHop, ~cv);
            ackOptional = true;
            return true;
        }
    }

    /*
     * Next priority: Send VRAM data.
     *
     * Normally this comes from a general-purpose VideoBuffer, though we
     * do have some special-purpose ways to send VRAM data without a
     * VideoBuffer.
     */

    if (UNLIKELY(CubeSlots::sendShutdown & cv)) {
        /*
         * Ask the cube to shut down (it will disconnect)
         *
         * Use a much lower than usual retry count, by disabling software retries,
         * so that this disconnect doesn't cause a noticeable hiccup in communications
         * with other cubes.
         *
         * Special case: if we're shutting down (as guessed by checking our
         * cube range), we can stand to wait the normal timeout to ensure
         * cubes get shut down along with the base.
         */

        codec.encodeShutdown(tx.packet);
        ASSERT(!tx.packet.isFull());
        if (!(CubeSlots::minUserCubes == 0 && CubeSlots::maxUserCubes == 0))
            tx.numSoftwareRetries = 0;

    } else if (UNLIKELY(CubeSlots::sendStipple & cv)) {
        // Send a stipple pattern
        codec.encodeStipple(tx.packet, vbuf);
        Atomic::And(CubeSlots::sendStipple, ~cv);
        ASSERT(!tx.packet.isFull());

    } else if (LIKELY(0 == (CubeSlots::vramPaused & cv))) {
        // Normal updates from VideoBuffer

        if (codec.encodeVRAM(tx.packet, vbuf)) {
            // Finished flushing Video Buffer. Maybe trigger a render.

            if (paintControl.vramFlushed(this)) {
                if (!codec.encodeVRAM(tx.packet, vbuf)) {
                    // Didn't have enough room to flush the trigger. More work to do!
                    idle = false;
                }
            }

        } else if (tx.packet.isFull()) {
            // Not done, and we filled up the packet. We have more work to do later!
            idle = false;
        }
    }

    /*
     * Second priority: Download assets to flash
     *
     * If the loader is asking for a flash escape, this means the rest of the packet
     * is owned by the asset loader. We can't write any non-flash data after the
     * flashEscape.
     */

    if (AssetLoader::getActiveCubes() & cv) {
        // Loading is in progress

        // Keep the radio from sleeping as long as we're loading assets.
        idle = false;

        if (AssetLoader::needFlashPacket(id()) && codec.escFlash(tx.packet)) {
            // Loader has data to send. Send an escape, and be done with this packet.
            AssetLoader::produceFlashPacket(id(), tx.packet);
            return true;
        }

        // Otherwise, maybe the loader needs a full ACK before it can make progress?
        if (AssetLoader::needFullACK(id()) && codec.escRequestAck(tx.packet))
            return true;
    }

    /*
     * Low priority: Sensor time synchronization
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
        codec.escTimeSync(tx.packet, calculateTimeSync());
        tx.noAck = true;    // just throw it out there UDP style
        return true;
    }

    /*
     * Last priority: Radio power management
     *
     * If we don't have any more work to do after this packet, tell the cube
     * to put its radio to sleep for a short time. We will remember when the
     * cube is expected to wake up, and we also relinquish transmit bandwidth
     * until then.
     */

    if (idle && getVersion() >= CUBE_FEATURE_NAP) {
        unsigned napTicks = suggestNapTicks();
        if (codec.escRadioNap(tx.packet, napTicks)) {
            // How long will we be asleep for? Naps are measured in units of 32.768 kHz ticks.
            // Give the cube 1ms extra, so it can wake up before we start talking to it.
            napDeadline = now + SysTime::msTicks(1) + (uint32_t(SysTime::hzTicks(32768)) * napTicks);
            return true;
        }
    }

    // Finalize this packet. Must be last.
    codec.endPacket(tx.packet);

    return true;
}

void CubeSlot::radioEmptyAcknowledge()
{
    ackOptional = false;

    applyPendingChannelHop();
}

void CubeSlot::radioAcknowledge(const PacketBuffer &packet)
{
    ackOptional = false;

    applyPendingChannelHop();

    // ACKs are always at least one byte.
    if (packet.len < 1) {
        ASSERT(0 && "Empty ACK packet. Radio bug?");
        return;
    }

    const RF_ACKType *ack = reinterpret_cast<const RF_ACKType*>(packet.bytes);

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
        if (loadACK)
            AssetLoader::ackData(id(), loadACK);
    }

    if (packet.len >= offsetof(RF_ACKType, accel) + sizeof ack->accel) {
        // Has valid accelerometer data.

        // Has the state changed at all? If not, don't bother storing it.
        if (memcmp(lastACK.accel, ack->accel, sizeof lastACK.accel)) {

            // Notify userspace about the immediate update
            Event::setCubePending(Event::PID_CUBE_ACCELCHANGE, id());

            // If userspace has subscribed to high-frequency updates, write to its MotionBuffer
            if (motionWriter.hasBuffer()) {
                motionWriter.write(MotionUtil::captureAccelState(*ack, getVersion()),
                                   SysTime::ticks());
            }
        }
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
            IdleTimeout::reset();
        }

        // Is this a flash reset ACK?
        if ((lastACK.neighbors[1] ^ ack->neighbors[1]) & NB1_FLAG_FLS_RESET) {
            AssetLoader::ackReset(id());
        }

        // Trigger a rescan of all neighbors, during event dispatch
        Event::setCubePending(Event::PID_NEIGHBORS, id());
    }

    if (packet.len >= offsetof(RF_ACKType, battery_v) + sizeof ack->battery_v) {
        // Packet has a valid battery voltage. Dispatch an event, if it's changed.

        if (lastACK.battery_v != ack->battery_v) {
            const _SYSCubeID cid = id();
            Event::setCubePending(Event::PID_CUBE_BATTERY, cid);

            // normally, lastACK only gets udpated below but we need the new value now
            // in order to update BatteryMonitor
            lastACK.battery_v = ack->battery_v;
            LOG(("new cube battery level %d, id %d\n", lastACK.battery_v, cid));
            BatteryMonitor::onCapture(getScaledBatteryV(), cid);
        }
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
    // If an ACK was required, disconnect the cube. Otherwise, ignore.
    if (!ackOptional) {
        disconnect();
    } else {
        /*
         * In this case, it's likely that we sent a hop command that didn't
         * get ACKed. This is feasible since we're hopping to avoid interference
         * anyway. So, we hope that the cube received the packet and just the
         * ACK got dropped, and apply the new channel.
         */
        applyPendingChannelHop();
    }

    ackOptional = false;
}

uint64_t CubeSlot::getHWID() const
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
    /*
     * Queries are theoretically a general-purpose protocol element,
     * but currently there's only one type of query (CRC) and it's
     * managed by the AssetLoader.
     *
     * If other query types were implemented, we would need a
     * dispatch layer that would allow us to map individual query
     * IDs to subsystems, either dynamically or statically.
     */

    AssetLoader::queryResponse(id(), packet);
}

unsigned CubeSlot::suggestNapTicks()
{
    /*
     * We don't have any more work to do immediately after the current
     * packet. How long should we ask the cube to sleep for? Returns
     * a result in units of 32.768 kHz ticks, up to a maximum of 0xFFFF.
     *
     * Right now there is only one input feeding into this calculation:
     * The suggested motion capture rate from userspace. If we have such
     * a suggestion, use it.
     *
     * Otherwise, we'll use a default minimum rate which polls at 60 Hz.
     */

    // Waiting on this cube? Don't sleep!
    if (bit() & CubeSlots::waitingOnCubes)
        return 0;

    // Maximum is 60 Hz
    unsigned maxRate = _SYS_MOTION_TIMESTAMP_HZ / 60;

    // Look for a motion buffer
    unsigned motionRate = motionWriter.getBufferRate();

    // Actual rate is the fastest of the two
    unsigned rate = MIN(maxRate, motionRate);

    // Convert from motion ticks to CLKLF ticks (approximate)
    unsigned napTicks = rate * (32768 / _SYS_MOTION_TIMESTAMP_HZ);

    return napTicks;
}
