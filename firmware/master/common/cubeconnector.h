/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_CONNECTOR_H
#define _CUBE_CONNECTOR_H

#include <protocol.h>
#include "radio.h"
#include "ringbuffer.h"
#include "flash_syslfs.h"


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
    static void init();

    // RadioManager callbacks
    static void radioProduce(PacketTransmission &tx);
    static void radioAcknowledge(const PacketBuffer &packet);
    static void radioEmptyAcknowledge();
    static void radioTimeout();

private:
    enum State {
        PairingFirstContact     = 0,
        PairingFirstVerify,
        PairingFinalVerify      = PairingFirstVerify + 3,
        PairingBeginHop,
        HopConfirm,
        ReconnectFirstContact,
        ReconnectAltFirstContact,
    };

    static uint8_t neighborKey;
    static _SYSPseudoRandomState prng;

    static SysLFS::PairingIDRecord savedPairings;
    static BitVector<SysLFS::NUM_PAIRINGS> reconnectQueue;

    static RadioAddress pairingAddr;
    static RadioAddress connectionAddr;
    static RadioAddress reconnectAddr;

    static uint8_t txState;
    static RingBuffer<RadioManager::FIFO_DEPTH, uint8_t, uint8_t> rxState;
    static uint8_t pairingPacketCounter;
    static uint8_t hwid[HWID_LEN];
    static _SYSCubeID cubeID;

    static void setNeighborKey(unsigned k);
    static void nextNeighborKey();
    static void refillReconnectQueue();
    static bool popReconnectQueue();

    static bool chooseConnectionAddr();
    static void produceRadioHop(PacketBuffer &buf);
};

#endif
