/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF51822 radio
 */

#ifndef _NRF51822_H
#define _NRF51822_H

#include "board.h"
#include "swd.h"

namespace nrf51822 {
    extern SWDMaster btle_swd;
    void test();
}

#endif // _NRF51822_H
