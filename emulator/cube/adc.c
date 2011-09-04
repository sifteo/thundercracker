/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "adc.h"
#include "emulator.h"

#define ADCCON1_PWRUP		0x80
#define ADCCON1_BUSY		0x40
#define ADCCON1_CHSEL_MASK	0x3C
#define ADCCON1_CHSEL_SHIFT	2
#define ADCCON2_CONT		0x20
#define ADCCON2_RATE_MASK	0x1C
#define ADCCON2_TACQ_MASK	0x03
#define ADCCON3_RESOL_MASK	0xC0
#define ADCCON3_RLJUST		0x20

struct {
    uint16_t inputs[16];
    int triggered;
    int conversion_timer;
    int conversion_channel;
    int period_timer;
} adc;


void adc_init(void)
{
    memset(&adc, 0, sizeof adc);
}

void adc_start(void)
{
    if (!adc.period_timer)
	adc.triggered = 1;
}

void adc_set_input(int index, uint16_t value16)
{
    adc.inputs[index] = value16;
}

static int adc_conversion_nsec(uint8_t *regs)
{
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

static int adc_rate_hz(uint8_t *regs)
{
    switch (regs[REG_ADCCON2] & ADCCON2_RATE_MASK) {
    default:
    case 0x00:  return 2000;
    case 0x04:  return 4000;
    case 0x08:  return 8000;
    case 0x0C:  return 16000;
    }
}

static void adc_store_result(uint8_t *regs, uint16_t result16)
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

int adc_tick(uint8_t *regs)
{
    int irq = 0;

    // Powered down?
    if (!(regs[REG_ADCCON1] & ADCCON1_PWRUP))
	return 0;

    if (adc.period_timer) {
	if (!--adc.period_timer)
	    adc.triggered = 1;
    }	    

    if (adc.triggered && !adc.conversion_timer) {
	// Start conversion
	adc.triggered = 0;
	adc.conversion_timer = NSEC_TO_CYCLES(adc_conversion_nsec(regs));
	adc.conversion_channel = (regs[REG_ADCCON1] & ADCCON1_CHSEL_MASK) >> ADCCON1_CHSEL_SHIFT;
    }

    if (adc.conversion_timer) {
	// Busy in a conversion

	adc.conversion_timer--;
	if (adc.conversion_timer) {
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
		adc.period_timer = HZ_TO_CYCLES(adc_rate_hz(regs));
		adc.period_timer -= NSEC_TO_CYCLES(adc_conversion_nsec(regs));
	    }

	    adc_store_result(regs, adc.inputs[adc.conversion_channel]);
	}
    }

    return irq;
}

