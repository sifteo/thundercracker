/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <math.h>
#include <protocol.h>
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
#include "noise.h"

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
    static double bitErrorRates[MAX_RF_CHANNEL + 1];
    static SysTime::Ticks lastNoiseUpdate;

    void trace();
    unsigned retryCount();
    bool testPacketLoss(unsigned bytes, unsigned channel);
    void updateRadioNoise(double noiseAmount);

    // total transmission attempts
    unsigned maxTries() {
        return (1 + unsigned(buf.ptx.numHardwareRetries))
             * (1 + unsigned(buf.ptx.numSoftwareRetries));
    }
}

bool RadioMC::testPacketLoss(unsigned bytes, unsigned channel)
{
    /*
     * Are we losing data due to RF noise? True if we're dropping
     * the packet, false if not.
     *
     * This handles loss of either the original packet or the ACK
     * by using an estimated bit error rate for the current channel,
     * which we update periodically using a time-variant noise function.
     *
     * The probability of dropping a packet depends on the bit error rate
     * and the packet's length. The bit error rate itself is the probability
     * that one bit will be corrupted. Its inverse is the probability
     * of a successful bit. Raising this probability to the power of
     * the packet's length, in bits, is the probability that no bit has
     * been disturbed.
     */

    ASSERT(channel <= MAX_RF_CHANNEL);
    double ber = RadioMC::bitErrorRates[buf.ptx.dest->channel];
    double successProbability = pow(1.0 - ber, bytes * 8);
    int threshold = successProbability * RAND_MAX;
    return rand() > threshold;
}

void RadioMC::updateRadioNoise(double noiseAmount)
{
    /*
     * Update the radio noise profile periodically. When we change
     * the noise profile, draw a graph of the new noise amounts to stdout.
     */

    if (!noiseAmount)
        return;

    SysTime::Ticks now = SysTime::ticks();
    if (now - lastNoiseUpdate < SysTime::msTicks(500))
        return;
    lastNoiseUpdate = now;

    // Time scale for Perlin noise
    double fNow = now / double(SysTime::sTicks(60));

    LOG(("NOISE: Radio noise spectrum summary:\n"));

    for (unsigned channel = 0; channel < arraysize(bitErrorRates); ++channel) {

        double perlin = Noise::perlin2D(fNow, channel / double(MAX_RF_CHANNEL), 10);

        // Exponentiated, so we have high peaks infrequently
        double exPerlin = powf(fabs(perlin), 4.0);

        // Arbitrary conversion to BER
        double ber = std::min(noiseAmount * exPerlin, 1.0);

        // Summarize the noise by printing a representative sample of channels
        if (!(channel & 7)) {
            LOG(("NOISE:    ch[%02x] BER=%.5f ", channel, ber));
            for (double x = 0; x < exPerlin; x += 1e-2)
                LOG(("#"));
            LOG(("\n"));
        }

        bitErrorRates[channel] = ber;
    }
}

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

unsigned RadioMC::retryCount()
{
    /*
     * Return the number of retries expended for the current transmission.
     * Don't count the single original attempt.
     */

    return maxTries() - 1 - buf.triesRemaining;
}

void SystemMC::doRadioPacket()
{
    /*
     * It is time to try sending one radio packet. Do a retry, if we already have
     * a transmission in progress, or start a new transmission.
     */

    RadioMC::Buffer &buf = RadioMC::buf;

    if (RadioManager::isRadioEnabled() && buf.triesRemaining == 0) {
        // Get a new packet

        memset(&buf, 0, sizeof buf);
        buf.ptx.init();
        buf.ptx.packet.bytes = buf.packet.payload;
        buf.prx.bytes = buf.reply.payload;

        // MC firmware produces a packet
        RadioManager::produce(buf.ptx);
        ASSERT(buf.ptx.dest != NULL);
        buf.packet.len = buf.ptx.packet.len;

        /*
         * As we don't model ACKs very closely, simply treat noAck packets
         * as though they simply have no retries.
         */
        if (buf.ptx.noAck == true) {
            buf.ptx.numHardwareRetries = 0;
            buf.ptx.numSoftwareRetries = 0;
        }
        
        ASSERT(buf.ptx.numHardwareRetries <= PacketTransmission::MAX_HARDWARE_RETRIES);
        buf.triesRemaining = RadioMC::maxTries();
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
     *
     * XXX: We don't yet model ACK loss separately, just dropping the
     *      original packet. To model ACK loss properly, we'd need to
     *      also take into account the nRF's packet ID counters.
     */

    RadioMC::updateRadioNoise(sys->opt_radioNoise);

    sys->getCubeSync().beginEventAt(radioPacketDeadline, mThreadRunning);

    if (RadioManager::isRadioEnabled()) {
        bool dropped = sys->opt_radioNoise &&
            RadioMC::testPacketLoss(buf.packet.len, buf.ptx.dest->channel);

        Cube::Hardware *cube = getCubeForAddress(buf.ptx.dest);

        buf.ack = cube && cube->isRadioClockRunning()
            && !dropped && cube->spi.radio.handlePacket(buf.packet, buf.reply);
        buf.ackCube = cube ? cube->id() : -1;
    }

    radioPacketDeadline += MCTiming::TICKS_PER_PACKET;
    sys->getCubeSync().endEvent(radioPacketDeadline);

    if (RadioManager::isRadioEnabled()) {

        --buf.triesRemaining;

        if (sys->opt_radioTrace)
            RadioMC::trace();

        if (buf.ack) {
            // Send response, and we're done

            if (buf.reply.len) {
                buf.prx.len = buf.reply.len;
                RadioManager::ackWithPacket(buf.prx, RadioMC::retryCount());
            } else {
                RadioManager::ackEmpty(RadioMC::retryCount());
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
    RadioManager::enableRadio();
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
