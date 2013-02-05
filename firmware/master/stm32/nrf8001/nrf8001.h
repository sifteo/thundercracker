/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2013 Sifteo, Inc. All rights reserved.
 */

/*
 * Driver for the Nordic nRF8001 Bluetooth Low Energy controller
 */

#ifndef _NRF8001_H
#define _NRF8001_H

#include "spi.h"
#include "gpio.h"

class NRF8001 {
public:
    NRF8001(GPIOPin _reqn,
            GPIOPin _rdyn,
            SPIMaster _spi)
        : reqn(_reqn), rdyn(_rdyn), spi(_spi)
          {}

    static NRF8001 instance;

    struct ACICommandBuffer {
        uint8_t length;
        uint8_t cmd[31];
    };

    struct ACIEventBuffer {
        uint8_t debug;
        uint8_t length;
        uint8_t event[30];
    };

    void init();
    void isr();

 private:
    GPIOPin reqn;
    GPIOPin rdyn;
    SPIMaster spi;

    ACICommandBuffer txBuffer;
    ACIEventBuffer rxBuffer;

    static void staticSpiCompletionHandler();
    void onSpiComplete();
};

#endif
