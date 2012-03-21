/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_SPI_H
#define _STM32_SPI_H

#include <stdint.h>
#include "hardware.h"
#include "gpio.h"


class SPIMaster {
 public:
    SPIMaster(volatile SPI_t *_hw,
              GPIOPin _csn,
              GPIOPin _sck,
              GPIOPin _miso,
              GPIOPin _mosi)
        : hw(_hw), csn(_csn), sck(_sck), miso(_miso), mosi(_mosi) {}

    void init();
    void begin();
    void end();

    uint8_t transfer(uint8_t b);
    void transfer(const uint8_t *txbuf, uint8_t *rxbuf, unsigned len);
    void transferTable(const uint8_t *table);

    void transferDma(const uint8_t *txbuf, uint8_t *rxbuf, unsigned len);
    void txDma(const uint8_t *txbuf, unsigned len);
    void rxDma(uint8_t *rxbuf, unsigned len);

    bool dmaInProgress() const;
  
 private:
    volatile SPI_t *hw;
    volatile DMAChannel_t *dmaRxChan;
    volatile DMAChannel_t *dmaTxChan;
    GPIOPin csn;
    GPIOPin sck;
    GPIOPin miso;
    GPIOPin mosi;

    static void dmaCallback(void *p, uint8_t flags);
};

#endif
