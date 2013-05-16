/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include "btprotocol.h"
#include "macros.h"

BTProtocol BTProtocol::instance;


void BTProtocolCallbacks::onConnect()
{
    BTProtocol &instance = BTProtocol::instance;
    instance.connected = true;
}

void BTProtocolCallbacks::onDisconnect()
{
    BTProtocol &instance = BTProtocol::instance;
    instance.connected = false;
}

void BTProtocolCallbacks::onReceiveData(uint8_t *buffer, unsigned length)
{
    // Implement me!
}

unsigned BTProtocolCallbacks::onProduceData(uint8_t *buffer)
{
    // Implement me!    
    return 0;
}
