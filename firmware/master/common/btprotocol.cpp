/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include "btprotocol.h"
#include "macros.h"

// xxx testing only
#include "homebutton.h"
#include "cube.h"


void BTProtocolHandler::onConnect()
{
    // Implement me!
}

void BTProtocolHandler::onDisconnect()
{
    // Implement me!
}

void BTProtocolHandler::onReceiveData(uint8_t *buffer, unsigned length)
{
    // Implement me!
}

unsigned BTProtocolHandler::onProduceData(uint8_t *buffer)
{
    // XXX: For throughput testing, we always send as much data as we can.

    for (unsigned i = 0; i < MAX_DATA_LEN; ++i) {
        buffer[i] = i;
    }

    // To illustrate the latency we're getting, stuff home button status
    // and Cube 0's accelerometer data in there too.

    _SYSByte4 accel = CubeSlot::getInstance(0).getAccelState();
    buffer[0] = HomeButton::isPressed();
    buffer[10] = accel.x;
    buffer[11] = accel.y;
    buffer[12] = accel.z;

    // We always have more data
    requestProduceData();

    return MAX_DATA_LEN;
}
