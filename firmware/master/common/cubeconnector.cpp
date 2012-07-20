/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include "macros.h"
#include "cubeconnector.h"


void CubeConnector::radioProduce(PacketTransmission &tx)
{
    static const RadioAddress addr = {
        111, RF_PAIRING_ADDRESS
    };

    tx.dest = &addr;
    tx.packet.len = 1;
    tx.packet.bytes[0] = 0xff;
}

void CubeConnector::radioAcknowledge(const PacketBuffer &packet)
{
}

void CubeConnector::radioTimeout()
{
}
