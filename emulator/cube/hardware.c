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
 *  - NOR Flash (Design supports up to 16 megabits, 21-bit addressing)
 *  - LCD Controller
 *
 * We're using three 8-bit I/O ports:
 *
 *  P0.7-0   Shared data bus, Flash + LCD
 *  P1.7-1   Flash A6-A0
 *  P1.0     LCD WRX
 *  P2.7     Latch A20-A14 from P1.7-1 on rising edge
 *  P2.6     Latch A13-A7 from P1.7-1 on rising edge
 *  P2.5     Flash OE
 *  P2.4     Flash CE
 *  P2.3     Flash WE
 *  P2.2     LCD CSX
 *  P2.1     LCD DCX
 *  P2.0     LCD TE Input
 */

#include <stdio.h>
#include <stdint.h>
#include "emu8051.h"
#include "hardware.h"
#include "emulator.h"
#include "lcd.h"
#include "flash.h"
#include "spi.h"
#include "radio.h"
#include "network.h"
#include "adc.h"

#define BUS_PORT	REG_P0
#define ADDR_PORT	REG_P1
#define CTRL_PORT	REG_P2

#define BUS_PORT_DIR	REG_P0DIR
#define ADDR_PORT_DIR	REG_P1DIR
#define CTRL_PORT_DIR	REG_P2DIR

static struct {
    uint8_t lat1;
    uint8_t lat2;
    uint8_t bus;
    uint8_t prev_ctrl_port;
    uint8_t flash_drv;

    int rfcken;
    struct spi_master radio_spi;
} hw;

// RFCON bits
#define RFCON_RFCKEN	0x04
#define RFCON_RFCSN	0x02
#define RFCON_RFCE	0x01


void hardware_init(struct em8051 *cpu)
{
    hw.lat1 = 0;
    hw.lat2 = 0;
    hw.bus = 0;
    hw.prev_ctrl_port = 0;

    cpu->mSFR[REG_P0DIR] = 0xFF;
    cpu->mSFR[REG_P1DIR] = 0xFF;
    cpu->mSFR[REG_P2DIR] = 0xFF;
    cpu->mSFR[REG_P3DIR] = 0xFF;
    
    cpu->mSFR[REG_SPIRCON0] = 0x01;
    cpu->mSFR[REG_SPIRCON1] = 0x0F;
    cpu->mSFR[REG_SPIRSTAT] = 0x03;
    cpu->mSFR[REG_SPIRDAT] = 0x00;
    cpu->mSFR[REG_RFCON] = RFCON_RFCSN;
 
    hw.radio_spi.callback = radio_spi_byte;
    hw.radio_spi.cpu = cpu;
    spi_init(&hw.radio_spi);

    network_init(opt_net_host, opt_net_port);
    flash_init(opt_flash_filename);
    radio_init(cpu);
    lcd_init();
    adc_init();
}

void hardware_exit(void)
{
    flash_exit();
    radio_exit();
    network_exit();
}

void hardware_gfx_tick(struct em8051 *cpu)
{
    // Port output values, pull-up when floating
    uint8_t bus_port = cpu->mSFR[BUS_PORT] | cpu->mSFR[BUS_PORT_DIR];
    uint8_t addr_port = cpu->mSFR[ADDR_PORT] | cpu->mSFR[ADDR_PORT_DIR];
    uint8_t ctrl_port = cpu->mSFR[CTRL_PORT] | cpu->mSFR[CTRL_PORT_DIR];

    // 7-bit address in high bits of p1
    uint8_t addr7 = addr_port >> 1;

    // Is the MCU driving any bit of the shared bus?
    uint8_t mcu_data_drv = cpu->mSFR[BUS_PORT_DIR] != 0xFF;

    struct flash_pins flashp = {
	/* addr    */ addr7 | ((uint32_t)hw.lat1 << 7) | ((uint32_t)hw.lat2 << 14),
	/* oe      */ ctrl_port & (1 << 5),
	/* ce      */ ctrl_port & (1 << 4),
	/* we      */ ctrl_port & (1 << 3),
	/* data_in */ hw.bus,
    };

    struct lcd_pins lcdp = {
	/* csx     */ ctrl_port & (1 << 2),
	/* dcx     */ ctrl_port & (1 << 1),
	/* wrx     */ addr_port & (1 << 0),
	/* rdx     */ ctrl_port & (1 << 0),
	/* data_in */ hw.bus,
    };

    flash_cycle(&flashp);
    lcd_cycle(&lcdp);

    /* Address latch write cycles, triggered by rising edge */

    if ((ctrl_port & 0x40) && !(hw.prev_ctrl_port & 0x40)) hw.lat1 = addr7;
    if ((ctrl_port & 0x80) && !(hw.prev_ctrl_port & 0x80)) hw.lat2 = addr7;
    hw.prev_ctrl_port = ctrl_port;

    /*
     * After every simulation cycle, resolve the new state of the
     * shared bus.  We update the bus once now, but flash memory may
     * additionally update more often (every tick).
     */
 
    switch ((mcu_data_drv << 1) | flashp.data_drv) {
    case 0:     /* Floating... */ break;
    case 1:     hw.bus = flash_data_out(); break;
    case 2:     hw.bus = bus_port; break;
    default:
	/* Bus contention! */
	cpu->except(cpu, EXCEPTION_BUS_CONTENTION);
    }
    
    hw.flash_drv = flashp.data_drv;  
    cpu->mSFR[BUS_PORT] = hw.bus;
}

void hardware_sfrwrite(struct em8051 *cpu, int reg)
{
    reg -= 0x80;
    switch (reg) {

    case BUS_PORT:
    case ADDR_PORT:
    case CTRL_PORT:
    case BUS_PORT_DIR:
    case ADDR_PORT_DIR:
    case CTRL_PORT_DIR:
	// Usually we only need to simulate the graphics subsystem when a relevant SFR write occurs
        hardware_gfx_tick(cpu);
        break;
 
    case REG_ADCCON1:
	adc_start();
	break;
            
    case REG_SPIRDAT:
        spi_write_data(&hw.radio_spi, cpu->mSFR[reg]);
        break;

    case REG_RFCON:
	hw.rfcken = !!(cpu->mSFR[reg] & RFCON_RFCKEN);
	radio_ctrl(!(cpu->mSFR[reg] & RFCON_RFCSN),   // Active low
		   !!(cpu->mSFR[reg] & RFCON_RFCE));  // Active high
	break;

    }
}

int hardware_sfrread(struct em8051 *cpu, int reg)
{
    reg -= 0x80;
    switch (reg) {
     
    case REG_SPIRDAT:
        return spi_read_data(&hw.radio_spi);
            
    default:    
        return cpu->mSFR[reg];
    }
}

void hardware_tick(struct em8051 *cpu)
{
    /* Update the LCD Tearing Effect line */

    cpu->mSFR[CTRL_PORT] &= 0xFE;
    if (lcd_te_tick())
	cpu->mSFR[CTRL_PORT] |= 0x01;

    /* Simulate interrupts */

    if (adc_tick(cpu->mSFR))
	cpu->mSFR[REG_IRCON] |= IRCON_MISC;

    if (spi_tick(&hw.radio_spi, &cpu->mSFR[REG_SPIRCON0]))
	cpu->mSFR[REG_IRCON] |= IRCON_RFSPI;

    if (hw.rfcken && radio_tick())
	cpu->mSFR[REG_IRCON] |= IRCON_RF;

    /* Other hardware with timers to update */

    flash_tick(cpu);
    if (hw.flash_drv)
	cpu->mSFR[BUS_PORT] = flash_data_out();
}
