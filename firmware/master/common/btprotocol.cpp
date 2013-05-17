/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

#include "btprotocol.h"
#include "macros.h"
#include "pause.h"
#include <stdint.h>
#include <string.h>

BTProtocol BTProtocol::instance;


void BTProtocolCallbacks::onConnect()
{
    BTProtocol::instance.flags.atomicClear(BTProtocol::PairingFlag);
    BTProtocol::instance.flags.atomicMark(BTProtocol::ConnectedFlag);
}

void BTProtocolCallbacks::onDisconnect()
{
    BTProtocol::instance.flags.atomicClear(BTProtocol::PairingFlag);
    BTProtocol::instance.flags.atomicClear(BTProtocol::ConnectedFlag);
}

void BTProtocolCallbacks::onDisplayPairingCode(const char *code)
{
    // Store the code in our long-lived buffer
    memcpy(BTProtocol::instance.pairingCode, code, BTProtocol::PAIRING_CODE_LEN);

    // Remember that we're in pairing mode until onConnect/onDisconnect happens
    BTProtocol::instance.flags.atomicMark(BTProtocol::PairingFlag);

    // Pause SVM, and display the pairing UI as long as PairingFlag is set.
    Pause::beginBluetoothPairing();
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
