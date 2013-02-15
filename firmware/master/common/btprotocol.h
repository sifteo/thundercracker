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

