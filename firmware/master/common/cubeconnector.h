/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_CONNECTOR_H
#define _CUBE_CONNECTOR_H

#include "radio.h"


/*
 * The CubeConnector is part of our radio transmission schedule, like
 * CubeSlot. Unlike CubeSlot, however, instead of talking to existing
 * cubes our job is to find and connect new cubes.
 *
 * This involves:
 *
 *   - Managing the base's current neighbor ID
 *   - Polling for neighbored cubes which we can pair
 *   - Polling for paired but disconnected cubes we can connect
 *
 * Note that the CubeConnector is always capable of producing radio
 * packets. Even if there are no paired cubes left to connect, we can
 * be polling for new pairable cubes.
 */

class CubeConnector {
 public:
    // RadioManager callbacks
    static void radioProduce(PacketTransmission &tx);
    static void radioAcknowledge(const PacketBuffer &packet);
    static void radioTimeout();

};

#endif
