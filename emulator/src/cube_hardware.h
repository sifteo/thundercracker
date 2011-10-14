/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_HARDWARE_H
#define _CUBE_HARDWARE_H

#include "cube_cpu.h"
#include "cube_radio.h"
#include "cube_adc.h"
#include "cube_accel.h"
#include "cube_spi.h"
#include "cube_i2c.h"
#include "cube_lcd.h"
#include "cube_flash.h"
#include "cube_neighbors.h"
#include "cube_cpu_core.h"
#include "cube_debug.h"
#include "vtime.h"

namespace Cube {

/*
 * This file simulates the hardware peripherals that we have directly
 * attached to the 8051:
 *
 *  - NOR Flash (Design supports up to 16 megabits, 21-bit addressing)
 *  - LCD Controller
 *
 * We have four 8-bit I/O ports:
 *
 *  P3.6     Flash OE
 *  P3.5     Flash WE
 *  P3.4     LCD Backlight / Reset
 *  P3.3     3.3v Enable
 *  P3.2     Latch A20-A14 from P1.7-1 on rising edge
 *  P3.1     Latch A13-A7 from P1.7-1 on rising edge
 *  P3.0     LCD DCX
 *
 *  P2.7-0   Shared data bus, Flash + LCD
 *
 *  P1.7     Neighbor Out 4
 *  P1.6     Neighbor Out 3
 *  P1.5     Neighbor Out 2
 *  P1.4     Neighbor In  (INT2)
 *  P1.3     Accelerometer SDA
 *  P1.2     Accelerometer SCL
 *  P1.1     Neighbor Out 1
 *  P1.0     Touch sense in (AIN8)
 *
 *  P0.7-1   Flash A6-A0
 *  P0.0     LCD WRX
 */

static const uint8_t ADDR_PORT       = REG_P0;
static const uint8_t MISC_PORT       = REG_P1;
static const uint8_t BUS_PORT        = REG_P2;
static const uint8_t CTRL_PORT       = REG_P3;

static const uint8_t ADDR_PORT_DIR   = REG_P0DIR;
static const uint8_t MISC_PORT_DIR   = REG_P1DIR;
static const uint8_t BUS_PORT_DIR    = REG_P2DIR;
static const uint8_t CTRL_PORT_DIR   = REG_P3DIR;

static const uint8_t CTRL_LCD_DCX    = (1 << 0);
static const uint8_t CTRL_FLASH_LAT1 = (1 << 1);
static const uint8_t CTRL_FLASH_LAT2 = (1 << 2);
static const uint8_t CTRL_FLASH_WE   = (1 << 5);
static const uint8_t CTRL_FLASH_OE   = (1 << 6);

// RFCON bits
static const uint8_t RFCON_RFCKEN    = 0x04;
static const uint8_t RFCON_RFCSN     = 0x02;
static const uint8_t RFCON_RFCE      = 0x01;


class Hardware {
 public:
    CPU::em8051 cpu;
    VirtualTime *time;
    LCD lcd;
    SPIBus spi;
    I2CBus i2c;
    ADC adc;
    Flash flash;
    Neighbors neighbors;
    
    /*
     * XXX: Currently requires a firmware image to be useful.
     *      We almost certainly don't want to ship a real firmware
     *      image as part of the SDK, assuming we want to keep
     *      our firmware a trade secret.
     *
     *      One alternative would be a static binary translation.
     *      First, scan the linker listings from the firmware to
     *      categorize all bytes that could be either (1) a jump
     *      target, or (2) a data table read via movc.
     *
     *      All bytes from (2) would be included verbatim in
     *      a baked-in firmware image. But this would *only* contain
     *      the data portion of the ROM. Mostly, this would be the
     *      ROM graphics. But any other small tables would also be
     *      present in this memory.
     *
     *      Next, all possible jump points (including ISRs, and the
     *      reset vector) are used to start translating basic blocks.
     *      Each basic block starts at a jump target, and ends at
     *      an outgoing branch. When we hit a branch, we mark the
     *      one or two target addresses as jump targets as well.
     *
     *      It's nice if this translation produces efficient code, but
     *      for security it's actually most important that the
     *      translation is difficult or impossible to reverse.
     *      Obviously we can't be including a copy of all instructions
     *      just in case they get used as a movc source address. So,
     *      we clearly need to separate the code and the data.
     *  
     *      Conveniently, the basic block translation itself is also
     *      hard to losslessly reverse. If we were translating
     *      individual instructions with no inter-instruction
     *      optimizations, it would concievably be easy to build a
     *      pattern matcher that converts each instruction back to its
     *      original form. But if we translate indivisible groups of
     *      instructions into C code, compile them to x86, then let
     *      the optimizer take a crack at them, the resulting code
     *      will be hard to predict, and will not necessarily map 1:1
     *      back to a single original instruction sequence.
     *
     *      As far as implementation: I'm expecting we'll have a
     *      build-time script (Python, C, whatever) that reads the
     *      firmware's .hex and .rst files, and generates C++ source
     *      code. This translated firmware image would consist of
     *      an actual firmware binary, containing only static data,
     *      plus a drop-in replacement for cpu_opcodes.cpp which
     *      in fact executes basic blocks instead of single opcodes.
     *      Interrupt latency becomes higher, but otherwise the
     *      resulting simulation should function in the same way.
     */

    bool init(VirtualTime *masterTimer,
              const char *firmwareFile, const char *flashFile);
    void exit();

    ALWAYS_INLINE bool tick() {
        bool cpuTicked = CPU::em8051_tick(&cpu);
        hardwareTick();
        return cpuTicked;
    }

    void lcdPulseTE() {
        lcd.pulseTE(hwDeadline);
    }

    void setAcceleration(float xG, float yG);
    void setTouch(float amount);

    bool isDebugging();

 private:

    ALWAYS_INLINE void hardwareTick()
    {
        /*
         * A big chunk of our work can happen less often than every
         * clock cycle, as determined by a simple deadline tracker.
         */

        if (hwDeadline.hasPassed())
            hwDeadlineWork();

        /*
         * A few small things currently have to happen per-tick
         */

        neighbors.tick(cpu);
        if (LIKELY(flash_drv))
            cpu.mSFR[BUS_PORT] = flash.dataOut();
    }

    void hwDeadlineWork();

    ALWAYS_INLINE void graphicsTick()
    {
        /*
         * Update the graphics (LCD and Flash) bus. Only happens in
         * response to relevant I/O port changes, not on every clock tick.
         */
        
        // Port output values, pull-up when floating
        uint8_t bus_port = cpu.mSFR[BUS_PORT] | cpu.mSFR[BUS_PORT_DIR];
        uint8_t addr_port = cpu.mSFR[ADDR_PORT] | cpu.mSFR[ADDR_PORT_DIR];
        uint8_t ctrl_port = cpu.mSFR[CTRL_PORT] | cpu.mSFR[CTRL_PORT_DIR];

        // 7-bit address in high bits of p1
        uint8_t addr7 = addr_port >> 1;

        // Is the MCU driving any bit of the shared bus?
        uint8_t mcu_data_drv = cpu.mSFR[BUS_PORT_DIR] != 0xFF;

        Flash::Pins flashp = {
            /* addr    */ addr7 | ((uint32_t)lat1 << 7) | ((uint32_t)lat2 << 14),
            /* oe      */ ctrl_port & CTRL_FLASH_OE,
            /* ce      */ 0,
            /* we      */ ctrl_port & CTRL_FLASH_WE,
            /* data_in */ bus,
        };

        LCD::Pins lcdp = {
            /* csx     */ 0,
            /* dcx     */ ctrl_port & CTRL_LCD_DCX,
            /* wrx     */ addr_port & 1,
            /* rdx     */ 0,
            /* data_in */ bus,
        };

        flash.cycle(&flashp);
        lcd.cycle(&lcdp);
    
        /* Address latch write cycles, triggered by rising edge */

        if ((ctrl_port & CTRL_FLASH_LAT1) && !(prev_ctrl_port & CTRL_FLASH_LAT1)) lat1 = addr7;
        if ((ctrl_port & CTRL_FLASH_LAT2) && !(prev_ctrl_port & CTRL_FLASH_LAT2)) lat2 = addr7;
        prev_ctrl_port = ctrl_port;

        /*
         * After every simulation cycle, resolve the new state of the
         * shared bus.  We update the bus once now, but flash memory may
         * additionally update more often (every tick).
         */
        
        switch ((mcu_data_drv << 1) | flashp.data_drv) {
        case 0:     /* Floating... */ break;
        case 1:     bus = flash.dataOut(); break;
        case 2:     bus = bus_port; break;
        default:
            /* Bus contention! */
            cpu.except(&cpu, CPU::EXCEPTION_BUS_CONTENTION);
        }
        
        flash_drv = flashp.data_drv;  
        cpu.mSFR[BUS_PORT] = bus;
    }

    static void except(CPU::em8051 *cpu, int exc);
    static void sfrWrite(CPU::em8051 *cpu, int reg);
    static int sfrRead(CPU::em8051 *cpu, int reg);

    TickDeadline hwDeadline;

    uint8_t lat1;
    uint8_t lat2;
    uint8_t bus;
    uint8_t prev_ctrl_port;
    uint8_t flash_drv;
    uint8_t rfcken;
};

};  // namespace Cube

#endif
