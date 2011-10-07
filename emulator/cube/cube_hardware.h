/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_HARDWARE_H
#define _CUBE_HARDWARE_H

#include "emu8051.h"
#include "radio.h"
#include "adc.h"
#include "accel.h"
#include "spi.h"
#include "i2c.h"
#include "lcd.h"
#include "flash.h"


class CubeHardware {
 public:
    em8051 cpu;
    LCD lcd;
    SPIBus spi;
    I2CBus i2c;
    ADC adc;
    Flash flash;

    uint64_t clocks;
    
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

    bool init(const char *firmwareFile, const char *flashFile,
              const char *netHost, const char *netPort);
    void exit();

 private:
    void hardwareTick();
    void graphicsTick();

    static void except(em8051 *cpu, int exc);
    static void sfrWrite(em8051 *cpu, int reg);
    static int sfrRead(em8051 *cpu, int reg);

    uint8_t lat1;
    uint8_t lat2;
    uint8_t bus;
    uint8_t prev_ctrl_port;
    uint8_t flash_drv;
    uint8_t iex3;
    uint8_t rfcken;
};

#endif
