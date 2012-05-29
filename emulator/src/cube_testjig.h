/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Simulated I2C factory test device
 */

#ifndef _CUBE_TESTJIG_H
#define _CUBE_TESTJIG_H

#include <vector>
#include <list>
#include "tinythread.h"

namespace Cube {


class I2CTestJig {
 public:
    I2CTestJig() : enabled(false) {}

    void setEnabled(bool e) {
        enabled = e;
    }

    void getACK(std::vector<uint8_t> &buffer) {
        // Return the last completed ack, not the one currently in progress.
        tthread::lock_guard<tthread::mutex> guard(mutex);
        buffer = ackPrevious;
    }

    void write(const uint8_t *bytes, unsigned count) {
        tthread::lock_guard<tthread::mutex> guard(mutex);
        while (count) {
            packetBuffer.push_back(*bytes);
            count--;
            bytes++;
        }
    }

    void i2cStart() {
        captureAck();
        state = enabled ? S_I2C_ADDRESS : S_IDLE;
    }
    
    void i2cStop() {
        captureAck();
        state = S_IDLE;
    }

    uint8_t i2cWrite(uint8_t byte) {
        switch (state) {

        case S_I2C_ADDRESS: {
            tthread::lock_guard<tthread::mutex> guard(mutex);

            if ((byte & 0xFE) == deviceAddress) {
                // Begin a test packet
                state = (byte & 1) ? S_READ_PACKET : S_WRITE_ACK;
            } else {
                // Not us
                state = S_IDLE;
            }
            break;
        }
            
        case S_WRITE_ACK: {
            tthread::lock_guard<tthread::mutex> guard(mutex);
            ackBuffer.push_back(byte);
            break;
        }

        default:
            break;
        }

        return state != S_IDLE;
    }

    uint8_t i2cRead(uint8_t ack) {
        uint8_t result = 0xff;

        switch (state) {

        case S_READ_PACKET: {
            tthread::lock_guard<tthread::mutex> guard(mutex);

            // NB: If empty, return a sentinel packet of [ff].
            if (!packetBuffer.empty()) {
                result = packetBuffer.front();
                packetBuffer.pop_front();
            }
            break;
        }

        default:
            break;
        }

        return result;
    }

 private:
    static const uint8_t deviceAddress = 0xAA;

    bool enabled;
    std::vector<uint8_t> ackBuffer;     // Local to emulator thread
    std::vector<uint8_t> ackPrevious;   // Protected by 'mutex'
    std::list<uint8_t> packetBuffer;    // Protected by 'mutex'
    tthread::mutex mutex;

    enum {
        S_IDLE,
        S_I2C_ADDRESS,
        S_WRITE_ACK,
        S_READ_PACKET,
    } state;
    
    void captureAck() {
        // Double-buffer the last full ACK packet we received.
        // Only applicable if this was an ACK write packet at all.

        tthread::lock_guard<tthread::mutex> guard(mutex);
        if (!ackBuffer.empty()) {
            ackPrevious = ackBuffer;
            ackBuffer.clear();
        }
    }
};


};  // namespace Cube

#endif
