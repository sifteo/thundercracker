/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

/*
 * Implements the hardware-specific BTProtocol interface using a concrete
 * Bluetooth driver like the NRF8001. If Bluetooth is not supported, this
 * file is a stub implementation of BTProtocolHardware.
 */

#include "nrf8001/nrf8001.h"
#include "board.h"
#include "btprotocol.h"

/*
 * Nordic nRF8001
 */

#ifdef HAVE_NRF8001

NRF8001 NRF8001::instance(NRF8001_REQN_GPIO,
                          NRF8001_RDYN_GPIO,
                          SPIMaster(&NRF8001_SPI,
                                    NRF8001_SCK_GPIO,
                                    NRF8001_MISO_GPIO,
                                    NRF8001_MOSI_GPIO,
                                    staticSpiCompletionHandler));

#if BOARD == BOARD_TC_MASTER_REV2
// on rev3, this is handled in ISR_EXTI9_5 - see exti.cpp
IRQ_HANDLER ISR_FN(NRF8001_EXTI_VEC)()
{
    NRF8001::instance.isr();
}
#endif

bool BTProtocolHardware::isAvailable()
{
    return true;
}

void BTProtocolHardware::requestProduceData()
{
    NRF8001::instance.requestTransaction();
}

/*
 * No Bluetooth
 */

#else

bool BTProtocolHardware::isAvailable()
{
    return false;
}

void BTProtocolHardware::requestProduceData()
{
    // Do nothing
}

#endif
