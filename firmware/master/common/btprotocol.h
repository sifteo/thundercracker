/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#ifndef BTPROTOCOL_H
#define BTPROTOCOL_H

#include "macros.h"
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
 * Any hardware-specific Bluetooth LE driver which backs the BTProtocolHandler
 * must implement a "Sifteo Base" GATT service with the following UUID:
 *
 * 566d0001-3c6f-8621-06d3-c14d4768bd75
 *
 * This service contains at least the following characteristics:
 *
 * 1. Data IN (Base -> Host), Notify pipe, max data length 20 bytes.
 *    UUID: 566d0002-3c6f-8621-06d3-c14d4768bd75
 *
 * 2. Data OUT (Host -> Base), Write-without-response pipe, max data length 20 bytes.
 *    UUID: 566d0003-3c6f-8621-06d3-c14d4768bd75
 *
 * 3. System Version, Readable characteristic, fixed length of 4 bytes.
 *    UUID: 566d0004-3c6f-8621-06d3-c14d4768bd75
 *    This equals the version returned by _SYS_version(), in little endian.
 */

class BTProtocolHandler {
public:

    static const unsigned MAX_DATA_LEN = 20;

    // Event handlers. ISR context!
    static void onConnect();
    static void onDisconnect();
    static void onReceiveData(uint8_t *buffer, unsigned length);
    static unsigned onProduceData(uint8_t *buffer);

    /*
     * Implemented by hardware-specific code. May be called on ISR or Task context.
     * This asks for onProduceData() to be invoked at least once.
     */
    static void requestProduceData();

};

#endif

