/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#ifndef BTPROTOCOL_H
#define BTPROTOCOL_H

#include "macros.h"
#include "bits.h"
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
    static const unsigned MAX_DATA_LEN = 20;
    static const unsigned PAIRING_CODE_LEN = 6;

    static bool isConnected() {
        return instance.flags.test(ConnectedFlag);
    }

    static bool isPairingInProgress() {
        return instance.flags.test(PairingFlag);
    }

    static const char *getPairingCode() {
        return instance.pairingCode;
    }

private:
    enum Flags {
        ConnectedFlag,
        PairingFlag,

        NUM_FLAGS   // Must be last
    };

    friend class BTProtocolCallbacks;
    static BTProtocol instance;

    BitVector<NUM_FLAGS> flags;
    char pairingCode[PAIRING_CODE_LEN];
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

