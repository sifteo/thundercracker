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

    // threshold to engage in a reconnection sequence with a cube.
    static const unsigned MIN_RECONNECT_BATTERY_LEVEL = 0x50;

    static void init();

    static ALWAYS_INLINE void enableReconnect() {
        reconnectEnabled = true;
    }

    static ALWAYS_INLINE void disableReconnect() {
        reconnectEnabled = false;
    }

    // RadioManager callbacks
    static void radioProduce(PacketTransmission &tx);
    static void radioAcknowledge(const PacketBuffer &packet);
    static void radioEmptyAcknowledge();
    static void radioTimeout();

    static void unpair(_SYSCubeID cid);

    // Callback for Tasks::CubeConnector
    static void task();

private:
    enum State {
        PairingFirstContact     = 0,
        PairingFirstVerify,
        PairingFinalVerify      = PairingFirstVerify + 3,
        PairingBeginHop,
        HopConfirm,
        ReconnectFirstContact,
        ReconnectAltFirstContact,
        ReconnectBeginHop,
    };

    enum TaskWorkItems {
        TaskRecyclePairings,
        TaskSavePairingID,
        TaskSavePairingMRU,

        NUM_WORK_ITEMS,         // Must be last
    };

    // number of soft retries used for both pairing & reconnection.
    static const unsigned CUBECONNECTOR_SOFT_RETRIES = 32;

    static uint8_t neighborKey;
    static bool reconnectEnabled;

    static SysLFS::PairingIDRecord savedPairingID;
    static SysLFS::PairingMRURecord savedPairingMRU;
    static BitVector<SysLFS::NUM_PAIRINGS> reconnectQueue;
    static BitVector<SysLFS::NUM_PAIRINGS> recycleQueue;
    static BitVector<NUM_WORK_ITEMS> taskWork;

    static RadioAddress pairingAddr;
    static RadioAddress connectionAddr;
    static RadioAddress reconnectAddr;

    static uint8_t txState;
    static uint8_t rxState;
    static uint8_t pairingPacketCounter;
    static uint8_t hwid[HWID_LEN];
    static _SYSCubeID cubeID;
    static SysLFS::Key cubeRecord;

    static void setNeighborKey(unsigned k);
    static void nextNeighborKey();
    static void refillReconnectQueue();
    static bool popReconnectQueue();
    static void newCubeRecord();
    static bool hwidIsPaired(const uint8_t *id);

    static bool chooseConnectionAddr();
    static void produceRadioHop(PacketBuffer &buf);
};

#endif
