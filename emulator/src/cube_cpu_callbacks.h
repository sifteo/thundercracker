/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_CPU_CALLBACKS_H
#define _CUBE_CPU_CALLBACKS_H

#include "cube_cpu.h"
#include "cube_hardware.h"
#include "macros.h"

namespace Cube {
namespace CPU {

/*
 * This is the interface used by em8051 to pass I/O callbacks back
 * up to Cube::Hardware. This is a bit messy, since we really need the
 * invocation of SFR reads/writes to be optimizable at compile-time.
 *
 * So, instead of using a function pointer, we have a combination of
 * a small inline test plus a statically linked predicate function.
 * This lets gcc do some very aggressive constant folding in our SBT code.
 */

struct SFR {
  
    static ALWAYS_INLINE void writeInline(Cube::Hardware *self, CPU::em8051 *cpu, int reg)
    {
        switch (reg) {

        // Use some register reads as hints to wake up the hardware emulation clock
        case REG_SPIRCON0:
        case REG_SPIRCON1:
        case REG_W2CON0:
        case REG_W2CON1:
        case REG_TL0:
        case REG_TH0:
        case REG_TL1:
        case REG_TH1:
        case REG_TL2:
        case REG_TH2:
        case REG_T2CON:
            cpu->needHardwareTick = true;
            break;
        
        case REG_IEN0:
        case REG_IEN1:
        case REG_TCON:
        case REG_IRCON:
        case REG_S0CON:
            cpu->needHardwareTick = true;
            cpu->needInterruptDispatch = true;
            break;
        
        case BUS_PORT:
        case ADDR_PORT:
        case CTRL_PORT:
        case BUS_PORT_DIR:
        case ADDR_PORT_DIR:
        case CTRL_PORT_DIR:
            self->graphicsTick();
            break;

        case MISC_PORT:
        case MISC_PORT_DIR:
            self->neighbors.ioTick(self->cpu);
            break;
     
        case REG_ADCCON1:
            cpu->needHardwareTick = true;
            self->adc.start();
            break;
                
        case REG_SPIRDAT:
            cpu->needHardwareTick = true;
            self->spi.writeData(cpu->mSFR[reg]);
            break;

        case REG_RFCON: {
            uint8_t value = cpu->mSFR[reg];
            cpu->needHardwareTick = true;
            self->setRadioClockEnable(!!(value & RFCON_RFCKEN));
            self->spi.radio.radioCtrl(!!(value & RFCON_RFCSN),
                                      !!(value & RFCON_RFCE));
            break;
        }

        case REG_W2DAT:
            cpu->needHardwareTick = true;
            self->i2c.writeData(cpu, cpu->mSFR[reg]);
            break;

        case REG_DEBUG:
            self->debugByte();
            break;
            
        case REG_MD0:
        case REG_MD1:
        case REG_MD2:
        case REG_MD3:
        case REG_MD4:
        case REG_MD5:
        case REG_ARCON:
            self->mdu.write(*self->time, *cpu, reg - REG_MD0);
            break;
        
        case REG_RNGCTL:
            self->rng.controlWrite(*self->time, *cpu);
            break;
            
        case REG_PWRDWN:
            // XXX: Only handle deep sleep mode
            if (cpu->mSFR[reg] == 0x01)
                cpu->deepSleep = true;
            break;
        
        }
    }

    static ALWAYS_INLINE int readInline(Cube::Hardware *self, CPU::em8051 *cpu, int reg)
    {
        switch (reg) {

        // Use some register reads as hints to wake up the hardware emulation clock.
        case REG_SPIRSTAT:      // SPI rx polling
            cpu->needHardwareTick = true;
            break;
        
        case REG_SPIRDAT:
            cpu->needHardwareTick = true;
            return self->spi.readData();
              
        case REG_W2DAT:
            cpu->needHardwareTick = true;
            return self->i2c.readData(cpu);

        case REG_W2CON1:
            cpu->needHardwareTick = true;
            return self->i2c.readCON1(cpu);
            
        case REG_MD0:
        case REG_MD1:
        case REG_MD2:
        case REG_MD3:
        case REG_MD4:
        case REG_MD5:
        case REG_ARCON:
            return self->mdu.read(*self->time, *cpu, reg - REG_MD0);

        case REG_RNGCTL:
            return self->rng.controlRead(*self->time, *cpu);
        case REG_RNGDAT:
            return self->rng.dataRead(*self->time, *cpu);

        case BUS_PORT:
            cpu->needHardwareTick = true;
            return self->readFlashBus();

        }
        
        return cpu->mSFR[reg];    
    }

    static ALWAYS_INLINE void write(CPU::em8051 *cpu, int reg)
    {
        reg -= 0x80;
        Cube::Hardware *self = (Cube::Hardware*) cpu->callbackData;
        
        if (__builtin_constant_p(reg))
            writeInline(self, cpu, reg);
        else
            self->sfrWrite(reg);
    }

    static ALWAYS_INLINE int read(CPU::em8051 *cpu, int reg)
    {
        reg -= 0x80;
        Cube::Hardware *self = (Cube::Hardware*) cpu->callbackData;
     
        if (__builtin_constant_p(reg))
            return readInline(self, cpu, reg);
        else
            return self->sfrRead(reg);
    }

};  // struct SFR


/*
 * Nonvolatile Memory read/write entry points
 */
 
struct NVM {
    static int write(CPU::em8051 *cpu, uint16_t addr, uint8_t data);
    static uint8_t read(CPU::em8051 *cpu, uint16_t addr);
};


};  // namespace CPU
};  // namespace Cube

#endif
