/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include "btprotocol.h"
#include "flash_device.h"
#include "usbvolumemanager.h"
#include "macros.h"


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

    // We always have more data
    requestProduceData();

    return MAX_DATA_LEN;
}
