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
    static Dac instance;

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
        Timer6,
        Timer3,
        Timer7,
        Timer5,
        Timer2,
        Timer4,
        Exti9,
        SwTrig,
        TrigNone
    };

    void init();
    void configureChannel(int ch, Waveform waveform = WaveNone, uint8_t mask_amp = 0, Trigger trig = TrigNone, BufferMode buffmode = BufferEnabled);
    void enableChannel(int ch);
    void disableChannel(int ch);

    void writeDual(uint16_t data);
    void write(int ch, uint16_t data, DataFormat format = RightAlign12Bit);
};

#endif // DAC_H_
