/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef DMA_H_
#define DMA_H_

#include <stdint.h>
#include "hardware.h"

class Dma
{
public:
    typedef void (*DmaIsr_t)(void *p, uint32_t flags);

    static void registerHandler(volatile DMA_t *dma, int channel, DmaIsr_t func, void *param);
    static void unregisterHandler(volatile DMA_t *dma, int channel);

private:
    struct DmaHandler_t {
        DmaIsr_t isrfunc;
        void *param;
    };

    static void serveIsr(volatile DMA_t *dma, int ch, DmaHandler_t *handlers);

    static uint32_t Ch1Mask;
    static DmaHandler_t Ch1Handlers[7];

    static uint32_t Ch2Mask;
    static DmaHandler_t Ch2Handlers[5];

    friend void ISR_DMA1_Channel1();
    friend void ISR_DMA1_Channel2();
    friend void ISR_DMA1_Channel3();
    friend void ISR_DMA1_Channel4();
    friend void ISR_DMA1_Channel5();
    friend void ISR_DMA1_Channel6();
    friend void ISR_DMA1_Channel7();

    friend void ISR_DMA2_Channel1();
    friend void ISR_DMA2_Channel2();
    friend void ISR_DMA2_Channel3();
    friend void ISR_DMA2_Channel4();
    friend void ISR_DMA2_Channel5();
};

#endif // DMA_H_
