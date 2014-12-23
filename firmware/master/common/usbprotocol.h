/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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

#ifndef USBPROTOCOL_H
#define USBPROTOCOL_H

#include "macros.h"
#include "ioqueue.h"

#include <stdint.h>
#include <string.h>

struct USBProtocolMsg;

class USBProtocol {

public:
    enum SubSystem {
        Installer       = 0,
        FactoryTest     = 1,
        Profiler        = 2,
        Debugger        = 3,
        Logger          = 4,
        DesktopSync     = 5,
        RFPassThrough   = 6,
        User            = 7
    };

    static void dispatch(const USBProtocolMsg &m);
    static void onReceiveData(const USBProtocolMsg &m);
    static void requestUserINPacket();

    // handler for the UsbIN task
    static void inTask();

    static bool setUserQueues(SvmMemory::VirtAddr send, SvmMemory::VirtAddr receive);
    static const _SYSUsbCounters *getCounters() {
        return &instance.counters;
    }

private:
    static USBProtocol instance;

    typedef IoQueue<_SYSUsbPacket, _SYSUsbQueue> UsbQueue;

    UsbQueue userSendQueue;
    UsbQueue userReceiveQueue;

    _SYSUsbCounters counters;
};

struct USBProtocolMsg {

    static const unsigned MAX_LEN = 64;
    static const unsigned HEADER_BYTES = sizeof(uint32_t);
    static const unsigned MAX_PAYLOAD_BYTES = MAX_LEN - HEADER_BYTES;

    unsigned len;
    union {
        uint8_t bytes[MAX_LEN];
        struct {
            uint32_t header;
            uint8_t payload[MAX_PAYLOAD_BYTES];
        };
    };

    USBProtocolMsg() :
        len(0) {}

    /*
     * Highest 4 bits in the header specify the subsystem.
     */
    USBProtocolMsg(USBProtocol::SubSystem ss) {
        init(ss);
    }

    void init(USBProtocol::SubSystem ss) {
        len = sizeof(header);
        header = ss << 28;
    }

    USBProtocol::SubSystem subsystem() const {
        return static_cast<USBProtocol::SubSystem>(header >> 28);
    }

    unsigned bytesFree() const {
        return sizeof(bytes) - len;
    }

    unsigned payloadLen() const {
        return MAX(0, static_cast<int>(len - sizeof(header)));
    }

    void append(uint8_t b) {
        ASSERT(len < MAX_LEN);
        bytes[len++] = b;
    }

    void append(const uint8_t *src, unsigned count) {
        // Overflow-safe assertions
        ASSERT(len <= MAX_LEN);
        ASSERT(count <= MAX_LEN);
        ASSERT((len + count) <= MAX_LEN);

        memcpy(bytes + len, src, count);
        len += count;
    }

    template <typename T> T* zeroCopyAppend() {
        // Overflow-safe assertions
        ASSERT(len <= MAX_LEN);
        ASSERT(sizeof(T) <= MAX_LEN);
        ASSERT((len + sizeof(T)) <= MAX_LEN);

        T* result = reinterpret_cast<T*>(bytes + len);
        len += sizeof(T);
        return result;
    }

    template <typename T> T* castPayload() {
        return reinterpret_cast<T*>(&payload[0]);
    }

    template <typename T> const T* castPayload() const {
        return reinterpret_cast<const T*>(&payload[0]);
    }
};

#endif

