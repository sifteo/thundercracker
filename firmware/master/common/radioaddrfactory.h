/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _RADIO_ADDR_FACTORY_H
#define _RADIO_ADDR_FACTORY_H

#include <protocol.h>
#include "radio.h"
#include "prng.h"


/*
 * Algorithms for generating radio addresses
 */

class RadioAddrFactory {
public:

    static void random(RadioAddress &addr, _SYSPseudoRandomState &prng);
    static unsigned randomChannel(_SYSPseudoRandomState &prng, unsigned currentChannel = 0xff);

    static void fromHardwareID(RadioAddress &addr, uint64_t hwid);
    static void convertPrimaryToAlternateChannel(RadioAddress &addr);

private:

    // 802.11 g/n channels are 20MHz wide, nordic channels are 1MHz
    static const unsigned WIFI_CHANNEL_WIDTH = 20;

    static const uint8_t gf84[0x100];

    static ALWAYS_INLINE bool allowedByte(uint8_t b)
    {
        switch (b) {
            case 0x00:
            case 0x55:
            case 0xAA:
            case 0xFF:
                return false;
            default:
                return true;
        }
    }
};

#endif
