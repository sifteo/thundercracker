/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This file simulates the hardware peripherals that we have directly
 * attached to the 8051:
 *
 *  - NOR Flash (Design supports up to 8 MByte, 23-bit addressing)
 *  - LCD Controller
 *
 * We're using three 8-bit I/O ports:
 *
 *  P0.7-0   Shared data bus, Flash + LCD
 *  P1.7-1   Flash A6-A0
 *  P1.0     LCD WRX
 *  P2.7     Latch A22-A15
 *  P2.6     Latch A14-A7
 *  P2.5     Flash OE
 *  P2.4     Flash CE
 *  P2.3     Flash WE
 *  P2.2     LCD CSX
 *  P2.1     LCD DCX
 *  P2.0     LCD RDX
 */

#include <stdint.h>
#include "emu8051.h"
#include "hardware.h"
#include "emulator.h"
#include "lcd.h"
#include "flash.h"

/* Vendor-specific SFRs on the nRF24LE1 */
#define REG_P0DIR  (0x93 - 0x80)
#define REG_P1DIR  (0x94 - 0x80)
#define REG_P2DIR  (0x95 - 0x80)
#define REG_P3DIR  (0x96 - 0x80)

static uint8_t addr_latch_1;
static uint8_t addr_latch_2;
static uint8_t shared_bus;

void hardware_init(struct em8051 *cpu)
{
    addr_latch_1 = 0;
    addr_latch_2 = 0;
    shared_bus = 0;

    cpu->mSFR[REG_P0DIR] = 0xFF;
    cpu->mSFR[REG_P1DIR] = 0xFF;
    cpu->mSFR[REG_P2DIR] = 0xFF;
    cpu->mSFR[REG_P3DIR] = 0xFF;

    flash_init(opt_flash_filename);
    lcd_init();
}

void hardware_exit(void)
{
    flash_exit();
    lcd_exit();
}

void hardware_sfrwrite(struct em8051 *cpu, int reg)
{
    // Port output values, pull-up when floating
    uint8_t p0 = cpu->mSFR[REG_P0] | cpu->mSFR[REG_P0DIR];
    uint8_t p1 = cpu->mSFR[REG_P1] | cpu->mSFR[REG_P1DIR];
    uint8_t p2 = cpu->mSFR[REG_P2] | cpu->mSFR[REG_P2DIR];

    // Is the MCU driving any bit of the shared bus?
    uint8_t mcu_data_drv = cpu->mSFR[REG_P0DIR] != 0xFF;

    struct flash_pins flashp = {
	.addr = ((p1 & 0xFE) >> 1) | (addr_latch_1 << 7) | (addr_latch_2 << 15),
	.oe = p2 & (1 << 5),
	.ce = p2 & (1 << 4),
	.we = p2 & (1 << 3),
	.data_in = shared_bus,
    };

    struct lcd_pins lcdp = {
	.csx = p2 & (1 << 2),
	.dcx = p2 & (1 << 1),
	.wrx = p1 & (1 << 0),
	.rdx = p2 & (1 << 0),
	.data_in = shared_bus,
    };

    flash_cycle(&flashp);
    lcd_cycle(&lcdp);

    /* After every simulation cycle, resolve the new state of the shared bus. */
   
    switch ((mcu_data_drv << 2) | (flashp.data_drv << 1) | lcdp.data_drv) {
    case 0:     /* Floating... */ break;
    case 1:  	shared_bus = lcdp.data_out; break;
    case 2:     shared_bus = flashp.data_out; break;
    case 4:     shared_bus = p0; break;
    default:
	/* Bus contention! */
	cpu->except(cpu, EXCEPTION_BUS_CONTENTION);
    }
    
    cpu->mSFR[REG_P0] = shared_bus;
}
