/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _STM32_SPI_H
#define _STM32_SPI_H

#include <stdint.h>
#include "hardware.h"
#include "gpio.h"
#include "dma.h"


class SPIMaster {
 public:

    enum Flags {
        // Clock divisors
        fPCLK_2     = (0 << 3),
        fPCLK_4     = (1 << 3),
        fPCLK_8     = (2 << 3),
        fPCLK_16    = (3 << 3),
        fPCLK_32    = (4 << 3),
        fPCLK_64    = (5 << 3),
        fPCLK_128   = (6 << 3),
        fPCLK_256   = (7 << 3),

        // SPI clocking mode
        fCPHA       = (1 << 0),
        fCPOL       = (1 << 1),

        // Bit order
        fLSBFIRST   = (1 << 7)
    };

    struct Config {
        Dma::Priority dmaRxPrio;
        unsigned flags;
        // others if we need them
    };

    typedef void (*CompletionCallback)();

    SPIMaster(volatile SPI_t *_hw,
              GPIOPin _sck,
              GPIOPin _miso,
              GPIOPin _mosi,
              CompletionCallback cb = 0)
        : hw(_hw), sck(_sck), miso(_miso), mosi(_mosi),
          completionCB(cb) {}

    void init(const Config & config);

    uint8_t transfer(uint8_t b);
    void transfer(const uint8_t *txbuf, uint8_t *rxbuf, unsigned len);

    void transferDma(const uint8_t *txbuf, uint8_t *rxbuf, unsigned len);
    void txDma(const uint8_t *txbuf, unsigned len);

 private:
    volatile SPI_t *hw;
    volatile DMAChannel_t *dmaRxChan;
    volatile DMAChannel_t *dmaTxChan;
    GPIOPin sck;
    GPIOPin miso;
    GPIOPin mosi;
    uint32_t dmaRxPriorityBits;

    CompletionCallback completionCB;
    static void dmaCallback(void *p, uint8_t flags);
};

#endif
