/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "system.h"
#include "system_mc.h"
#include "macros.h"
#include "radio.h"
#include "systime.h"
#include "cube.h"
#include "protocol.h"
#include "tasks.h"
#include "mc_timing.h"
#include "bits.h"

namespace RadioMC {

    struct Buffer {
        PacketTransmission ptx;
        PacketBuffer prx;
        Cube::Radio::Packet packet;
        Cube::Radio::Packet reply;
        bool ack;
        unsigned ackCube;
        unsigned triesRemaining;
    };

    static Buffer buf;
    static bool active;

    void trace();

};


void RadioMC::trace()
{
    LOG(("RADIO: %6dms %02x/%02x%02x%02x%02x%02x -- TX[%2d] ",
        int(SysTime::ticks() / SysTime::msTicks(1)),
        buf.ptx.dest->channel,
        buf.ptx.dest->id[4],
        buf.ptx.dest->id[3],
        buf.ptx.dest->id[2],
        buf.ptx.dest->id[1],
        buf.ptx.dest->id[0],
        buf.packet.len));

    // Nybbles in little-endian order. Except for arguments to escape
    // codes, the TX packets are always nybble streams.

    for (unsigned i = 0; i < sizeof buf.packet.payload; i++) {
        if (i < buf.packet.len) {
            uint8_t b = buf.packet.payload[i];
            LOG(("%x%x", b & 0xf, b >> 4));
        } else {
            LOG(("  "));
        }
    }

    // ACK data segmented into struct pieces

    if (buf.ack) {
        LOG((" -- Cube %d: ACK[%2d] ", buf.ackCube, buf.reply.len));
        for (unsigned i = 0; i < buf.reply.len; i++) {
            switch (i) {

                // First byte ('frame' et al) is always special
                case RF_ACK_LEN_FRAME:
                    LOG(("-"));
                    break;

                // ACK packet delimiters (omit for query responses)
                case RF_ACK_LEN_ACCEL:
                case RF_ACK_LEN_NEIGHBOR:
                case RF_ACK_LEN_FLASH_FIFO:
                case RF_ACK_LEN_BATTERY_V:
                case RF_ACK_LEN_HWID:
                    if (!(buf.reply.payload[0] & QUERY_ACK_BIT)) {
                        LOG(("-"));
                    }
                    break;
            }
            LOG(("%02x", buf.reply.payload[i]));
        }
        LOG(("\n"));
    } else {
        LOG((" -- TIMEOUT, #%d tries left\n", buf.triesRemaining));
    }
}


void SystemMC::doRadioPacket()
{
    /*
     * It is time to try sending one radio packet. Do a retry, if we already have
     * a transmission in progress, or start a new transmission.
     */

    RadioMC::Buffer &buf = RadioMC::buf;

    if (RadioMC::active && buf.triesRemaining == 0) {
        // Get a new packet

        memset(&buf, 0, sizeof buf);
        buf.ptx.init();
        buf.ptx.packet.bytes = buf.packet.payload;
        buf.prx.bytes = buf.reply.payload;

        // MC firmware produces a packet
        RadioManager::produce(buf.ptx);
        ASSERT(buf.ptx.dest != NULL);
        buf.packet.len = buf.ptx.packet.len;
        
        ASSERT(buf.ptx.numHardwareRetries <= PacketTransmission::MAX_HARDWARE_RETRIES);
        buf.triesRemaining = (1 + unsigned(buf.ptx.numHardwareRetries))
                           * (1 + unsigned(buf.ptx.numSoftwareRetries));
    }

    /*
     * Deliver it to the proper cube.
     *
     * Interaction with the cube simulation must take place
     * between beginEvent() and endEvent() only.
     *
     * Note that this causes us to sync the Cube thread's clock with
     * radioPacketDeadline, which slightly lags our 'ticks' counter,
     * which slightly lags the internal SvmCpu cycle count.
     *
     * The timestamp we give to endEvent() is the farthest we allow
     * the Cube thread to run asynchronously before waiting for us again.
     */

    sys->getCubeSync().beginEventAt(radioPacketDeadline, mThreadRunning);

    if (RadioMC::active) {
        Cube::Hardware *cube = getCubeForAddress(buf.ptx.dest);
        buf.ack = cube && cube->spi.radio.handlePacket(buf.packet, buf.reply);
        buf.ackCube = cube ? cube->id() : -1;
    }

    radioPacketDeadline += MCTiming::TICKS_PER_PACKET;
    sys->getCubeSync().endEvent(radioPacketDeadline);

    if (RadioMC::active) {

        --buf.triesRemaining;

        if (sys->opt_radioTrace)
            RadioMC::trace();

        if (buf.ack) {
            // Send response, and we're done

            if (buf.reply.len) {
                buf.prx.len = buf.reply.len;
                RadioManager::ackWithPacket(buf.prx);
            } else {
                RadioManager::ackEmpty();
            }

            buf.triesRemaining = 0;
            return;
        }

        if (!buf.triesRemaining) {
            // Out of retries
            RadioManager::timeout();
        }
    }
}

void Radio::init()
{
    RadioMC::active = true;
}

void Radio::heartbeat()
{
    // Nothing to do here in simulation yet
}

Cube::Hardware *SystemMC::getCubeForAddress(const RadioAddress *addr)
{
    uint64_t packed = addr->pack();

    for (unsigned i = 0; i < sys->opt_numCubes; i++) {
        Cube::Hardware &cube = sys->cubes[i];
        if (cube.spi.radio.getPackedRXAddr() == packed)
            return &cube;
    }

    return NULL;
}
