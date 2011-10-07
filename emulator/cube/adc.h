/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _ADC_H
#define _ADC_H

#include <stdint.h>
#include "reg8051.h"

class ADC {
 public:

    void init() {
        memset(this, 0, sizeof *this);
    }

    void start() {
        if (!period_timer)
            triggered = 1;
    }

    void setInput(int index, uint16_t value16) {
        inputs[index] = value16;
    }

    int tick(uint8_t *regs) {
        int irq = 0;

        // Powered down?
        if (!(regs[REG_ADCCON1] & ADCCON1_PWRUP))
            return 0;

        if (period_timer) {
            if (!--period_timer)
                triggered = 1;
        }

        if (triggered && !conversion_timer) {
            // Start conversion
            triggered = 0;
            conversion_timer = NSEC_TO_CYCLES(conversionNSec(regs));
            conversion_channel = (regs[REG_ADCCON1] & ADCCON1_CHSEL_MASK) >> ADCCON1_CHSEL_SHIFT;
        }

        if (conversion_timer) {
            // Busy in a conversion

            conversion_timer--;
            if (conversion_timer) {
                regs[REG_ADCCON1] |= ADCCON1_BUSY;
            } else {
                /*
                 * Just finished the conversion.
                 *
                 * We need to clear BUSY, raise an IRQ, scheudle another
                 * periodic timer if we're in continuous mode, and
                 * store/convert the result.
                 */

                regs[REG_ADCCON1] &= ~ADCCON1_BUSY;
                irq = 1;

                if (regs[REG_ADCCON2] & ADCCON2_CONT) {
                    period_timer = HZ_TO_CYCLES(rateHZ(regs));
                    period_timer -= NSEC_TO_CYCLES(conversionNSec(regs));
                }

                storeResult(regs, inputs[conversion_channel]);
            }
        }
        
        return irq;
    }
    
 private:
 
    static int conversionNSec(uint8_t *regs) {
        /*
         * How many clock cycles for a conversion?
         * This is based on Table 100 in the nRF24LE1 data sheet.
         */

        switch ((regs[REG_ADCCON2] & ADCCON2_TACQ_MASK) |
                (regs[REG_ADCCON3] & ADCCON3_RESOL_MASK)) {

            // tAcq = 0.75us
        default:
        case 0x00:  return 3000;
        case 0x40:  return 3200;
        case 0x80:  return 3400;
        case 0xC0:  return 3600;

            // tAcq = 3us
        case 0x01:  return 5300;
        case 0x41:  return 5400;
        case 0x81:  return 5600;
        case 0xC1:  return 5800;

            // tAcq = 12us
        case 0x02:  return 14300;
        case 0x42:  return 14400;
        case 0x82:  return 14600;
        case 0xC2:  return 14800;

            // tAcq = 36us
        case 0x03:  return 38300;
        case 0x43:  return 38400;
        case 0x83:  return 38600;
        case 0xC3:  return 38800;
        }
    }
    
    static int rateHZ(uint8_t *regs) {
        switch (regs[REG_ADCCON2] & ADCCON2_RATE_MASK) {
        default:
        case 0x00:  return 2000;
        case 0x04:  return 4000;
        case 0x08:  return 8000;
        case 0x0C:  return 16000;
        }
    }

    static void storeResult(uint8_t *regs, uint16_t result16)
    {
        switch (regs[REG_ADCCON3] & (ADCCON3_RESOL_MASK | ADCCON3_RLJUST)) {

            // Left justified, 6-bit
        case 0x00:
            regs[REG_ADCDATH] = (result16 >> 8) & 0xFC;
            regs[REG_ADCDATL] = 0;
            break;

            // Left justified, 8-bit
        case 0x40:
            regs[REG_ADCDATH] = result16 >> 8;
            regs[REG_ADCDATL] = 0;
            break;
            
            // Left justified, 10-bit
        case 0x80:
            regs[REG_ADCDATH] = result16 >> 8;
            regs[REG_ADCDATL] = result16 & 0xC0;
            break;

            // Left justified, 12-bit
        case 0xC0:
            regs[REG_ADCDATH] = result16 >> 8;
            regs[REG_ADCDATL] = result16 & 0xF0;
            break;

            // Right justified, 6-bit
        case 0x20:
            regs[REG_ADCDATH] = 0;
            regs[REG_ADCDATL] = result16 >> 10;
            break;

            // Right justified, 8-bit
        case 0x60:
            regs[REG_ADCDATH] = 0;
            regs[REG_ADCDATL] = result16 >> 8;
            break;

            // Right justified, 10-bit
        case 0xA0:
            regs[REG_ADCDATH] = result16 >> 14;
            regs[REG_ADCDATL] = result16 >> 6;
            break;

            // Right justified, 12-bit
        case 0xE0:
            regs[REG_ADCDATH] = result16 >> 12;
            regs[REG_ADCDATL] = result16 >> 4;
            break;
        }
    }

    uint16_t inputs[16];
    int triggered;
    int conversion_timer;
    int conversion_channel;
    int period_timer;

    static const uint8_t ADCCON1_PWRUP         = 0x80;
    static const uint8_t ADCCON1_BUSY          = 0x40;
    static const uint8_t ADCCON1_CHSEL_MASK    = 0x3C;
    static const uint8_t ADCCON1_CHSEL_SHIFT   = 2;
    static const uint8_t ADCCON2_CONT          = 0x20;
    static const uint8_t ADCCON2_RATE_MASK     = 0x1C;
    static const uint8_t ADCCON2_TACQ_MASK     = 0x03;
    static const uint8_t ADCCON3_RESOL_MASK    = 0xC0;
    static const uint8_t ADCCON3_RLJUST        = 0x20;
};
 
#endif
