/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
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

#ifndef IO_DEVICE_H
#define IO_DEVICE_H

#include <stdint.h>
#include "usbprotocol.h"

/*
 * Simple backend interface.
 * Expected instances are USB for hardware, and TCP for siftulator.
 */
class IODevice {
public:
    static const unsigned MAX_EP_SIZE = 64;
    static const unsigned MAX_OUTSTANDING_OUT_TRANSFERS = 32;

    static const unsigned SIFTEO_VID = 0x22fa;
    static const unsigned BASE_PID = 0x0105;
    static const unsigned BOOTLOADER_PID = 0x0115;

    virtual bool open(uint16_t vendorId, uint16_t productId, uint8_t interface = 0) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual int  processEvents(unsigned timeoutMillis = 0) = 0;

    virtual unsigned maxINPacketSize() const = 0;
    virtual unsigned numPendingINPackets() const = 0;
    virtual int readPacket(uint8_t *buf, unsigned maxlen, unsigned & rxlen) = 0;

    virtual unsigned maxOUTPacketSize() const = 0;
    virtual unsigned numPendingOUTPackets() const = 0;
    virtual int writePacket(const uint8_t *buf, unsigned len) = 0;
};

#endif // IO_DEVICE_H
