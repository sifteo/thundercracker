/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef USART_H_
#define USART_H_

#include "hardware.h"

class Usart
{
public:

    enum StopBits {
        Stop1 = 0,
        Stop0_5 = 1,
        Stop2 = 2,
        Stop1_5 = 3
    };
    Usart(volatile USART_t *hw) : uart(hw)
    {}
    void init(int rate, StopBits bits = Stop1);
    void deinit();

    void write(const char* buf, int size);
    void write(const char* buf);
    void read(char *buf, int size);

    void put(char c);
    char get();

private:
    volatile USART_t *uart;
};

#endif /* USART_H_ */
