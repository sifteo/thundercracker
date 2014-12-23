/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
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

    // Return the last completed ack, not the one currently in progress.
    void getACK(std::vector<uint8_t> &buffer) {
        tthread::lock_guard<tthread::mutex> guard(mutex);
        buffer = ackPrevious;
    }

    // Atomically add a set of byte(s) to send, next time the cube asks us for data.
    // This buffer is synchronusly added to our queue, between transactions.
    void write(const uint8_t *bytes, unsigned count) {
        tthread::lock_guard<tthread::mutex> guard(mutex);
        while (count) {
            writeNext.push_back(*bytes);
            count--;
            bytes++;
        }
    }

    void i2cStart() {
        syncPoint();
        state = enabled ? S_I2C_ADDRESS : S_IDLE;
    }
    
    void i2cStop() {
        syncPoint();
        state = S_IDLE;
    }

    uint8_t i2cWrite(uint8_t byte) {
        switch (state) {

        case S_I2C_ADDRESS: {
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
            // NB: If empty, return a sentinel packet of [ff].
            if (!writeBuffer.empty()) {
                result = writeBuffer.front();
                writeBuffer.pop_front();
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
    std::list<uint8_t> writeBuffer;     // Local to emulator thread

    std::vector<uint8_t> ackPrevious;   // Protected by 'mutex'
    std::list<uint8_t> writeNext;       // Protected by 'mutex'
    tthread::mutex mutex;

    enum {
        S_IDLE,
        S_I2C_ADDRESS,
        S_WRITE_ACK,
        S_READ_PACKET,
    } state;
    
    // Called on emulator thread, between packets
    void syncPoint() {
        tthread::lock_guard<tthread::mutex> guard(mutex);

        // Double-buffer the last full ACK packet we received.
        // Only applicable if this was an ACK write packet at all.
        if (!ackBuffer.empty()) {
            ackPrevious = ackBuffer;
            ackBuffer.clear();
        }

        // Atomically add the next write packet(s) to our queue
        writeBuffer.splice(writeBuffer.end(), writeNext);
    }
};


};  // namespace Cube

#endif
