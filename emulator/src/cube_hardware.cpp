/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cube_hardware.h"
#include "cube_debug.h"

namespace Cube {

bool Hardware::init(VirtualTime *masterTimer,
                    const char *firmwareFile, const char *flashFile)
{
    time = masterTimer;
    hwDeadline.init(time);

    lat1 = 0;
    lat2 = 0;
    bus = 0;
    prev_ctrl_port = 0;

    cpu.except = except;
    cpu.sfrread = sfrRead;
    cpu.sfrwrite = sfrWrite;
    cpu.callbackData = this;

    CPU::em8051_reset(&cpu, true);

    if (firmwareFile)
        CPU::em8051_load(&cpu, firmwareFile);
    else
        CPU::em8051_init_sbt(&cpu);

    cpu.mSFR[REG_P0DIR] = 0xFF;
    cpu.mSFR[REG_P1DIR] = 0xFF;
    cpu.mSFR[REG_P2DIR] = 0xFF;
    cpu.mSFR[REG_P3DIR] = 0xFF;
    
    cpu.mSFR[REG_SPIRCON0] = 0x01;
    cpu.mSFR[REG_SPIRCON1] = 0x0F;
    cpu.mSFR[REG_SPIRSTAT] = 0x03;
    cpu.mSFR[REG_SPIRDAT] = 0x00;
    cpu.mSFR[REG_RFCON] = RFCON_RFCSN;
 
    if (!flash.init(flashFile))
        return false;    
    spi.radio.init(&cpu);
    spi.init(&cpu);
    adc.init();
    i2c.init();
    lcd.init();
    neighbors.init();

    return true;
}

void Hardware::exit()
{
    flash.exit();
}

void Hardware::except(CPU::em8051 *cpu, int exc)
{
    Hardware *self = (Hardware*) cpu->callbackData;
    const char *name = CPU::em8051_exc_name(exc);

    if (cpu->traceFile)
        fprintf(cpu->traceFile, "EXCEPTION at 0x%04x: %s\n", cpu->mPC, name);

    if (self == Cube::Debug::cube) {
        Cube::Debug::emu_exception(cpu, exc);
    } else {
        fprintf(stderr, "EXCEPTION at 0x%04x: %s\n", cpu->mPC, name);
    }
}

void Hardware::sfrWrite(CPU::em8051 *cpu, int reg)
{
    Hardware *self = (Hardware*) cpu->callbackData;
    reg -= 0x80;
    uint8_t value = cpu->mSFR[reg];

    self->hwDeadline.setRelative(0);

    switch (reg) {

    case REG_IEN0:
    case REG_IEN1:
    case REG_TCON:
    case REG_IRCON:
    case REG_S0CON:
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
        self->adc.start();
        break;
            
    case REG_SPIRDAT:
        self->spi.writeData(value);
        break;

    case REG_RFCON:
        self->rfcken = !!(value & RFCON_RFCKEN);
        self->spi.radio.radioCtrl(!!(value & RFCON_RFCSN),
                                  !!(value & RFCON_RFCE));
        break;

    case REG_W2DAT:
        self->i2c.writeData(cpu, value);
        break;

    case REG_DEBUG:
        printf("DEBUG<%p>: %02x\n", self, value);
        break;

    }
}

int Hardware::sfrRead(CPU::em8051 *cpu, int reg)
{
    Hardware *self = (Hardware*) cpu->callbackData;
    reg -= 0x80;

    self->hwDeadline.setRelative(0);

    switch (reg) {
     
    case REG_SPIRDAT:
        return self->spi.readData();
          
    case REG_W2DAT:
        return self->i2c.readData(cpu);

    case REG_W2CON1:
        return self->i2c.readCON1(cpu);

    default:    
        return cpu->mSFR[reg];
    }
}

void Hardware::setAcceleration(float xG, float yG)
{
    /*
     * Set the cube's current acceleration, in G's. Scale it
     * according to the accelerometer's maximum range (ours is rated
     * at +/- 2G).
     */

    const float deviceAccelScale = 128.0 / 2.0;

    int x = xG * deviceAccelScale;
    int y = yG * deviceAccelScale;

    if (x < -128) x = -128;
    if (x > 127) x = 127;

    if (y < -128) y = -128;
    if (y > 127) y = 127;

    i2c.accel.setVector(x, y);
}

NEVER_INLINE void Hardware::hwDeadlineWork() 
{
    hwDeadline.reset();

    lcd.tick(hwDeadline, &cpu);
    adc.tick(hwDeadline, &cpu);
    spi.tick(hwDeadline, cpu.mSFR + REG_SPIRCON0, &cpu);
    i2c.tick(hwDeadline, &cpu);
    flash.tick(hwDeadline, &cpu);
    spi.radio.tick(rfcken, &cpu);
}

void Hardware::setTouch(float amount)
{
    /* XXX: Model this as a capacitance applied to the ADC */
}

bool Hardware::isDebugging()
{
    return this == Cube::Debug::cube;
}


};  // namespace Cube
