/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
    void writeHex(uint32_t value);
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
