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
 *  P3.6     Flash OE
 *  P3.5     Flash WE
 *  P3.2     Latch A20-A14 from P1.7-1 on rising edge
 *  P3.1     Latch A13-A7 from P1.7-1 on rising edge
 *  P3.0     LCD DCX
 *  P2.7-0   Shared data bus, Flash + LCD
 *  P0.7-1   Flash A6-A0
 *  P0.0     LCD WRX
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

#define ADDR_PORT       REG_P0
#define BUS_PORT        REG_P2
#define CTRL_PORT       REG_P3

#define ADDR_PORT_DIR   REG_P0DIR
#define BUS_PORT_DIR    REG_P2DIR
#define CTRL_PORT_DIR   REG_P3DIR

#define CTRL_LCD_DCX    (1 << 0)
#define CTRL_FLASH_LAT1 (1 << 1)
#define CTRL_FLASH_LAT2 (1 << 2)
#define CTRL_FLASH_WE   (1 << 5)
#define CTRL_FLASH_OE   (1 << 6)

#define CTRL_LCD_TE     0       // XXX: TE pin not available in our hardware

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
#define RFCON_RFCKEN    0x04
#define RFCON_RFCSN     0x02
#define RFCON_RFCE      0x01


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
        /* oe      */ ctrl_port & CTRL_FLASH_OE,
        /* ce      */ 0,
        /* we      */ ctrl_port & CTRL_FLASH_WE,
        /* data_in */ hw.bus,
    };

    struct lcd_pins lcdp = {
        /* csx     */ 0,
        /* dcx     */ ctrl_port & CTRL_LCD_DCX,
        /* wrx     */ addr_port & 1,
        /* rdx     */ 0,
        /* data_in */ hw.bus,
    };

    flash_cycle(&flashp);
    lcd_cycle(&lcdp);

    /* Address latch write cycles, triggered by rising edge */

    if ((ctrl_port & CTRL_FLASH_LAT1) && !(hw.prev_ctrl_port & CTRL_FLASH_LAT1)) hw.lat1 = addr7;
    if ((ctrl_port & CTRL_FLASH_LAT2) && !(hw.prev_ctrl_port & CTRL_FLASH_LAT2)) hw.lat2 = addr7;
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

    case REG_DEBUG:
        fprintf(stderr, "Debug: %02x\n", cpu->mSFR[reg]);
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

    cpu->mSFR[CTRL_PORT] &= ~CTRL_LCD_TE;
    if (lcd_te_tick())
        cpu->mSFR[CTRL_PORT] |= CTRL_LCD_TE;

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
