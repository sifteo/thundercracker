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

    // compatible with the return value from isr()
    enum Status {
        StatusOverrun   = (1 << 3),
        StatusRxed      = (1 << 5),
        StatusTxed      = (1 << 7)
    };

    enum StopBits {
        Stop1   = 0,
        Stop0_5 = 1,
        Stop2   = 2,
        Stop1_5 = 3
    };

    static Usart Dbg;

    Usart(volatile USART_t *hw) : uart(hw)
    {}
    void init(GPIOPin rx, GPIOPin tx, int rate, StopBits bits = Stop1);
    void deinit();

    void write(const uint8_t* buf, int size);
    void write(const char* buf);
    void writeHex(uint32_t value);
    void read(uint8_t *buf, int size);

    void put(char c);
    char get();

    uint16_t isr(uint8_t &byte);

private:
    volatile USART_t *uart;

};

#endif /* USART_H_ */
