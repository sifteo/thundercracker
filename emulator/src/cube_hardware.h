/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_HARDWARE_H
#define _CUBE_HARDWARE_H

#include <algorithm>
#include "cube_cpu.h"
#include "cube_radio.h"
#include "cube_adc.h"
#include "cube_accel.h"
#include "cube_spi.h"
#include "cube_i2c.h"
#include "cube_mdu.h"
#include "cube_rng.h"
#include "cube_lcd.h"
#include "cube_flash.h"
#include "cube_neighbors.h"
#include "cube_cpu_core.h"
#include "cube_debug.h"
#include "vtime.h"
#include "tracer.h"


namespace Cube {

/*
 * This file simulates the hardware peripherals that we have directly
 * attached to the 8051.
 */

static const uint8_t ADDR_PORT       = REG_P0;
static const uint8_t MISC_PORT       = REG_P1;
static const uint8_t BUS_PORT        = REG_P2;
static const uint8_t CTRL_PORT       = REG_P3;

static const uint8_t ADDR_PORT_DIR   = REG_P0DIR;
static const uint8_t MISC_PORT_DIR   = REG_P1DIR;
static const uint8_t BUS_PORT_DIR    = REG_P2DIR;
static const uint8_t CTRL_PORT_DIR   = REG_P3DIR;

static const uint8_t MISC_TOUCH      = (1 << 7);

static const uint8_t CTRL_LCD_DCX    = (1 << 0);
static const uint8_t CTRL_FLASH_LAT1 = (1 << 2);
static const uint8_t CTRL_FLASH_LAT2 = (1 << 1);
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
    MDU mdu;
    FlashStorage flashStorage;
    Flash flash;
    Neighbors neighbors;
    RNG rng;
    
    bool init(VirtualTime *masterTimer,
              const char *firmwareFile, const char *flashFile,
              bool wakeFromSleep);

    void exit();    
    void reset();

    ALWAYS_INLINE bool isSleeping() {
        return cpu.deepSleep;
    }

    ALWAYS_INLINE void tick(bool *cpuTicked=NULL) {
        if (!isSleeping()) {
            CPU::em8051_tick(&cpu, 1, cpu.sbt, cpu.mProfileData != NULL, Tracer::isEnabled(), cpu.mBreakpoint != 0, cpuTicked);
            hardwareTick();
        }
    }

    ALWAYS_INLINE unsigned tickFastSBT(unsigned tickBatch=1) {
        /*
         * Assume at compile-time that we're in SBT mode, and no debug features are active.
         * Also try to aggressively skip ticks when possible. The fastest code is code that never runs.
         * Returns the number of ticks that may be safely batched next time.
         * 
         * Also note that we calculate our remaining clock cycles using 32-bit math, and we blindly
         * truncate the output of remaining(). This is intentional; normally remaining() will fit in
         * 32 bits, but even if it does cause overflow the worst case is that we'll end up skipping
         * fewer ticks than possible. So, this is always safe, and it's highly important for performance
         * when we're running on 32-bit platforms.
         *
         * Assumes the caller has already checked isSleeping().
         */
        
        CPU::em8051_tick(&cpu, tickBatch, true, false, false, false, NULL);
        hardwareTick();
                    
        return std::min(std::min(cpu.mTickDelay, (unsigned)cpu.prescaler12),
                        (unsigned)hwDeadline.remaining());
    }

    void lcdPulseTE() {
        lcd.pulseTE(hwDeadline);
    }

    void setAcceleration(float xG, float yG, float zG);
    void setTouch(bool touching);

    bool isDebugging();
    void initVCD(VCDWriter &vcd);

    // SFR side-effects
    void sfrWrite(int reg);
    int sfrRead(int reg);
    void debugByte();
    void graphicsTick();
    
    ALWAYS_INLINE void setRadioClockEnable(bool e) {
        rfcken = e;
    }
    
    uint32_t getExceptionCount();
    void incExceptionCount();
    
    ALWAYS_INLINE uint8_t readFlashBus() {
        if (LIKELY(flash_drv))
            cpu.mSFR[BUS_PORT] = flash.dataOut();
        return cpu.mSFR[BUS_PORT];
    }    
    
 private:

    ALWAYS_INLINE void hardwareTick()
    {
        /*
         * A big chunk of our work can happen less often than every
         * clock cycle, as determined by a simple deadline tracker.
         *
         * We also trigger hardware writes via a simpler (and shorter
         * to inline) method, just by writing to a byte in the CPU
         * struct. This is used by the inline SFR callbacks
         * in cube_cpu_callbacks.h.
         *
         * We can safely assume that needHardwareTick is set by CPU code,
         * before hardwareTick executes, and that it won't be set during
         * hwDeadlineWork().
         */

        if (hwDeadline.hasPassed() || cpu.needHardwareTick)
            hwDeadlineWork();
    }

    int16_t scaleAccelAxis(float g)
    {
        /*
         * Scale a raw acceleration, in G's, and return the corresponding
         * two's complement accelerometer reading. Saturates at either extreme.
         */
         
        const int range = 1 << 15;
        const float fullScale = 2.0f;
        
        int scaled = g * (range / fullScale);
        int16_t truncated = scaled;
        
        if (scaled != truncated)
            truncated = scaled > 0 ? range - 1 : -range;
            
        return truncated;
    }

    void hwDeadlineWork();
    TickDeadline hwDeadline;

    uint8_t lat1;
    uint8_t lat2;
    uint8_t bus;
    uint8_t prev_ctrl_port;
    uint8_t flash_drv;
    uint8_t rfcken;
    
    uint32_t exceptionCount;
};

};  // namespace Cube

#endif
