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


void SystemMC::doRadioPacket()
{
    // Prepare buffers
    struct {
        PacketTransmission ptx;
        PacketBuffer prx;
        Cube::Radio::Packet packet;
        Cube::Radio::Packet reply;
        bool ack;
    } buf;
    
    memset(&buf, 0, sizeof buf);
    buf.ptx.init();
    buf.ptx.packet.bytes = buf.packet.payload;
    buf.prx.bytes = buf.reply.payload;

    // MC firmware produces a packet
    RadioManager::produce(buf.ptx);
    ASSERT(buf.ptx.dest != NULL);
    buf.packet.len = buf.ptx.packet.len;

    ASSERT(buf.ptx.numHardwareRetries <= PacketTransmission::MAX_HARDWARE_RETRIES);
    const unsigned numTries = (1 + unsigned(buf.ptx.numHardwareRetries))
                            * (1 + unsigned(buf.ptx.numSoftwareRetries));

    for (unsigned retry = 0; retry < numTries; ++retry) {
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
        Cube::Hardware *cube = getCubeForAddress(buf.ptx.dest);
        buf.ack = cube && cube->spi.radio.handlePacket(buf.packet, buf.reply);
        radioPacketDeadline += MCTiming::TICKS_PER_PACKET;
        sys->getCubeSync().endEvent(radioPacketDeadline);

        // Log this transaction
        if (sys->opt_radioTrace) {
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
                LOG((" -- Cube %d: ACK[%2d] ", cube->id(), buf.reply.len));
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
                LOG((" -- TIMEOUT, retry #%d\n", retry));
            }
        }

        // Send the response
        if (buf.ack) {
            if (buf.reply.len) {
                buf.prx.len = buf.reply.len;
                RadioManager::ackWithPacket(buf.prx);
            } else {
                RadioManager::ackEmpty();
            }
            return;
        }
    }
    
    // Out of retries
    RadioManager::timeout();
}

void Radio::init()
{
    // Nothing to do in simulation
}

void Radio::begin()
{
    // Nothing to do in simulation - hardware requires a delay between init
    // and the beginning of transmissions.
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
