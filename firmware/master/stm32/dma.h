/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef DMA_H_
#define DMA_H_

#include <stdint.h>
#include "hardware.h"

class Dma
{
public:
    typedef void (*DmaIsr_t)(void *p, uint8_t flags);

    enum Priority {
        LowPrio         = (0 << 12),
        MediumPrio      = (1 << 12),
        HighPrio        = (2 << 12),
        VeryHighPrio    = (3 << 12)
    };

    enum IsrFlags {
        Complete        = (1 << 1),
        HalfComplete    = (1 << 2),
        Error           = (1 << 3)
    };

    static volatile DMAChannel_t * initChannel(volatile DMA_t *dma, int channel, DmaIsr_t func, void *param);
    static void deinitChannel(volatile DMA_t *dma, int channel);

private:
    struct DmaHandler_t {
        DmaIsr_t isrfunc;
        void *param;
    };

    static void serveIsr(volatile DMA_t *dma, int ch, DmaHandler_t &handler);

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
