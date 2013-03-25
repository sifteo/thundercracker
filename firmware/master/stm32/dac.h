/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef DAC_H_
#define DAC_H_

#include "hardware.h"

class Dac
{
public:
    enum BufferMode {
        BufferEnabled,
        BufferDisabled
    };

    enum Waveform {
        WaveNone,
        WaveNoise,
        WaveTriangle
    };

    enum DataFormat {
        RightAlign12Bit,
        LeftAlign12Bit,
        RightAlign8Bit
    };

    enum Trigger {
        TIM6,
        TIM3,
        TIM7,
        TIM5,
        TIM2,
        TIM4,
        Exti9,
        SwTrig,
        TrigNone
    };

    static void init();
    static void configureChannel(int ch, Trigger trig = TrigNone,
        Waveform waveform = WaveNone, uint8_t mask_amp = 0, BufferMode buffmode = BufferEnabled);

    static void enableChannel(int ch);
    static void disableChannel(int ch);

    static void enableDMA(int ch);
    static void disableDMA(int ch);

    static void writeDual(uint16_t data);
    static void write(int ch, uint16_t data, DataFormat format = RightAlign12Bit);
    static volatile uint32_t *address(int ch, DataFormat format = RightAlign12Bit);
};

#endif // DAC_H_
