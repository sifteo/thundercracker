/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include "btprotocol.h"

/*
 * Siftulator doesn't yet support Bluetooth. This file contains stubs which
 * implement a fake Bluetooth device that's never available.
 */

bool BTProtocolHardware::isAvailable()
{
    return false;
}

void BTProtocolHardware::requestProduceData()
{
    // Nothing to do.
}
