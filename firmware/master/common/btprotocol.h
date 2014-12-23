/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2013 Sifteo, Inc.
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

#ifndef BTPROTOCOL_H
#define BTPROTOCOL_H

#include "macros.h"
#include "bits.h"
#include "tasks.h"
#include "btqueue.h"
#include <stdint.h>

/*
 * This is a hardware-agnostic delegate which handles Bluetooth events.
 *
 * We do not handle actual GATT characteristics here, since they're
 * managed in very different ways by different Bluetooth LE chipsets.
 * Here, we handle high-level connection state events and we handle
 * "data" packets which are sent via generic "Data IN" and "Data OUT"
 * GATT characteristics.
 */

/*
 * Any hardware-specific Bluetooth LE driver which backs the BTProtocol layer
 * must implement a "Sifteo Base" GATT service with the following UUID:
 *
 * 566d0001-3c6f-8621-06d3-c14d4768bd75
 *
 * This service contains at least the following characteristics:
 *
 * 1. Data IN (Base -> Host), Notify pipe, max data length 20 bytes.
 *    UUID: 566d0002-3c6f-8621-06d3-c14d4768bd75
 *
 * 2. Data OUT (Host -> Base), Write pipe with response, max data length 20 bytes.
 *    UUID: 566d0003-3c6f-8621-06d3-c14d4768bd75
 *
 * 3. System Version, Readable characteristic, fixed length of 4 bytes.
 *    UUID: 566d0004-3c6f-8621-06d3-c14d4768bd75
 *    This equals the version returned by _SYS_version(), in little endian.
 */


/**
 * Common BTProtocol object. Non-hardware-specific Bluetooth entry points,
 * implemented by btprotocol.cpp
 */

class BTProtocol {
public:
    static const unsigned PAIRING_CODE_LEN = 6;
    static const unsigned TYPE_MASK = 0x7F;
    static const unsigned TYPE_SYS = 0x80;

    // Tasks::BluetoothProtocol
    static void task();

    static bool setUserQueues(SvmMemory::VirtAddr send, SvmMemory::VirtAddr receive);
    static bool setUserState(SvmMemory::VirtAddr data, unsigned length);

    static bool isConnected() {
        return instance.flags.test(ConnectedFlag);
    }

    static bool isPairingInProgress() {
        return instance.flags.test(PairingFlag);
    }

    static void reportVolume() {
        instance.flags.atomicMark(ReportVolumeFlag);
        Tasks::trigger(Tasks::BluetoothProtocol);
    }

    static const char *getPairingCode() {
        return instance.pairingCode;
    }

    static const _SYSBluetoothCounters *getCounters() {
        return &instance.counters;
    }

private:
    enum Flags {
        ConnectedFlag,
        PairingFlag,
        ReportVolumeFlag,

        NUM_FLAGS   // Must be last
    };

    enum SysPackets {
        CurrentApp,
    };

    void handleSystemPacket(unsigned type, const uint8_t *data, unsigned length);
    unsigned produceSystemPacket(uint8_t *buffer);

    // task handlers
    ALWAYS_INLINE void captureCurrentVolume();

    friend class BTProtocolCallbacks;
    static BTProtocol instance;

    BitVector<NUM_FLAGS> flags;
    BTQueue userSendQueue;
    BTQueue userReceiveQueue;
    _SYSBluetoothCounters counters;
    char pairingCode[PAIRING_CODE_LEN];

    // :( unfortunately, we need somewhere to put data for host-bound
    // packets that cannot be generated in ISR context (ie, in onProduceData())
    // This size is currently driven by the maximum package str length we want to support.
    struct SysDataBuffer {
        uint8_t length;     // count in 'payload'
        uint8_t type;       // bluetooth packet type
        uint8_t *p;         // progress through 'payload'
        uint8_t payload[40];

        ALWAYS_INLINE void setLength(unsigned sz) {
            length = sz;
            p = payload;
        }

        ALWAYS_INLINE unsigned bytesToWrite() const {
            return length - (p - payload);
        }

        ALWAYS_INLINE unsigned writeTo(uint8_t *b) {
            unsigned chunk = MIN(bytesToWrite(), _SYS_BT_PACKET_BYTES);
            b[0] = type | TYPE_SYS;
            memcpy(&b[1], p, chunk);
            p += chunk;
            ASSERT((p - payload) < sizeof payload);
            return chunk + sizeof(type);
        }
    };

    SysDataBuffer sysData;
};


/**
 * Hardware-specific backend entry points. Callable on ISR or Task context.
 */

class BTProtocolHardware {
public:
    /// Is Bluetooth available?
    static bool isAvailable();

    /// Ask for onProduceData() to be invoked at least once.
    static void requestProduceData();
};


/**
 * BTProtocol callbacks, invoked by the hardware-specific backend.
 * These event handlers run in ISR context.
 */

class BTProtocolCallbacks {
public:
    static void onConnect();
    static void onDisconnect();
    static void onDisplayPairingCode(const char *code);

    static void onReceiveData(uint8_t *buffer, unsigned length);
    static unsigned onProduceData(uint8_t *buffer);
};

#endif
