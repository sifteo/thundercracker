
#include "dac.h"

//static
Dac Dac::instance;

void Dac::init()
{
    RCC.APB1ENR |= (1 << 29); // enable dac peripheral clock
}

// channels: 1 or 2
void Dac::configureChannel(int ch, Waveform waveform, uint8_t mask_amp, Trigger trig, BufferMode buffmode)
{
    uint16_t reg =  (mask_amp << 8) |
                    (waveform << 6) |
                    (trig == TrigNone ? 0 : (trig << 3) | (1 << 2)) | // if not none, both enable the trigger and configure it
                    (buffmode << 1);
    DAC.CR |= (reg << ((ch - 1) * 16));
}

void Dac::enableChannel(int ch)
{
    DAC.CR |= (1 << ((ch - 1) * 16));
}

void Dac::disableChannel(int ch)
{
    DAC.CR &= ~(1 << ((ch - 1) * 16));
}

void Dac::write(int ch, uint16_t data, DataFormat format)
{
    volatile DACChannel_t &dc = DAC.channels[ch - 1];
    dc.DHR[format] = data;
}

// TODO - this is only 8-bit dual, support other formats/alignments as needed
void Dac::writeDual(uint16_t data)
{
    DAC.DHR8RD = data;
}

