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

#ifndef USART_H_
#define USART_H_

#include "hardware.h"

class GPIOPin;

class Usart
{
public:

    static const uint16_t STATUS_OVERRUN    = (1 << 3);
    static const uint16_t STATUS_RXED       = (1 << 5);
    static const uint16_t STATUS_TXED       = (1 << 7);

    static Usart Dbg;

    enum StopBits {
        Stop1 = 0,
        Stop0_5 = 1,
        Stop2 = 2,
        Stop1_5 = 3
    };

    typedef void (*CompletionCallback)();

    Usart(volatile USART_t *hw, CompletionCallback txCB = 0, CompletionCallback rxCB = 0)
        : uart(hw),
          txCompletionCB(txCB),
          rxCompletionCB(rxCB)
    {}

    void init(GPIOPin rx, GPIOPin tx, int rate, bool dma = false, StopBits bits = Stop1);
    void deinit();

    void write(const uint8_t* buf, int size);
    void write(const char* buf);
    void writeHex(uint32_t value, unsigned numDigits = 8);
    void writeHexBytes(const void *data, int size);
    void read(uint8_t *buf, int size);

    void writeDma(const uint8_t *buf, unsigned len);
    void readDma(const uint8_t *buf, unsigned len);

    void put(char c);
    char get();

    uint16_t isr(uint8_t &byte);

private:
    volatile USART_t *uart;
    volatile DMAChannel_t *dmaRxChan;
    volatile DMAChannel_t *dmaTxChan;

    const CompletionCallback txCompletionCB;
    const CompletionCallback rxCompletionCB;

    static void dmaTXCallback(void *p, uint8_t flags);
    static void dmaRXCallback(void *p, uint8_t flags);
};

#endif /* USART_H_ */
