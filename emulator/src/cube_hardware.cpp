/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
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

#define ADDR_PORT       REG_P0
#define MISC_PORT       REG_P1
#define BUS_PORT        REG_P2
#define CTRL_PORT       REG_P3

#define ADDR_PORT_DIR   REG_P0DIR
#define MISC_PORT_DIR   REG_P1DIR
#define BUS_PORT_DIR    REG_P2DIR
#define CTRL_PORT_DIR   REG_P3DIR

#define CTRL_LCD_DCX    (1 << 0)
#define CTRL_FLASH_LAT1 (1 << 1)
#define CTRL_FLASH_LAT2 (1 << 2)
#define CTRL_FLASH_WE   (1 << 5)
#define CTRL_FLASH_OE   (1 << 6)

// XXX: We have support in software for TE, but the IO pin isn't wired up currently
#define CTRL_LCD_TE     0

// RFCON bits
#define RFCON_RFCKEN    0x04
#define RFCON_RFCSN     0x02
#define RFCON_RFCE      0x01

#include "cube_hardware.h"
#include "cube_debug.h"

namespace Cube {


bool Hardware::init(VirtualTime *masterTimer,
                    const char *firmwareFile, const char *flashFile)
{
    time = masterTimer;

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

void Hardware::graphicsTick()
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

void Hardware::hardwareTick()
{
    uint8_t intexp = cpu.mSFR[REG_INTEXP];

    /*
     * Update the LCD Tearing Effect line
     */

    cpu.mSFR[CTRL_PORT] &= ~CTRL_LCD_TE;
    if (lcd.tick())
        cpu.mSFR[CTRL_PORT] |= CTRL_LCD_TE;

    /*
     * Simulate peripheral interrupts
     */

    if (adc.tick(cpu.mSFR))
        cpu.mSFR[REG_IRCON] |= IRCON_MISC;

    if (spi.tick(&cpu.mSFR[REG_SPIRCON0]))
        cpu.mSFR[REG_IRCON] |= IRCON_RFSPI;

    if (rfcken && spi.radio.tick())
        cpu.mSFR[REG_IRCON] |= IRCON_RF;

    // I2C can be routed to iex3 using INTEXP
    uint8_t nextIEX3 = i2c.tick(&cpu) && (intexp & 0x04);    

    // Neighbor sensors generate an IRQ indirectly via GPINT2
    neighbors.tick(cpu);

    /*
     * External interrupts: GPIOs. Only one pin can be selected
     * for use as an Interrupt From Pin (IFP) source at a time.
     */

    uint8_t nextIFP;
    switch (intexp & 0x38) {
    case 0x08:  nextIFP = cpu.mSFR[REG_P1] & (1 << 2);  break;   // GPINT0
    case 0x10:  nextIFP = cpu.mSFR[REG_P1] & (1 << 3);  break;   // GPINT1
    case 0x20:  nextIFP = cpu.mSFR[REG_P1] & (1 << 4);  break;   // GPINT2
    default:    nextIFP = 0;
    };

    if (cpu.mSFR[REG_TCON] & TCONMASK_IT0) {
        // Falling edge
        if (!nextIFP && ifp)
            cpu.mSFR[REG_TCON] |= TCONMASK_IE0;
    } else {
        // Low level
        if (!nextIFP)
            cpu.mSFR[REG_TCON] |= TCONMASK_IE0;
    }
    ifp = nextIFP;

    /*
     * External interrupts: iex3
     */

    if (cpu.mSFR[REG_T2CON] & 0x40) {
        // Rising edge
        if (nextIEX3 && !iex3)
            cpu.mSFR[REG_IRCON] |= IRCON_SPI;
    } else {
        // Falling edge
        if (!nextIEX3 && iex3)
            cpu.mSFR[REG_IRCON] |= IRCON_SPI;
    }
    iex3 = nextIEX3;
        
    /*
     * Other hardware with timers to update
     */

    flash.tick(&cpu);
    if (flash_drv)
        cpu.mSFR[BUS_PORT] = flash.dataOut();
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
    switch (reg) {

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
        printf("Debug: %02x\n", value);
        break;

    }
}

int Hardware::sfrRead(CPU::em8051 *cpu, int reg)
{
    Hardware *self = (Hardware*) cpu->callbackData;
    reg -= 0x80;
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

bool Hardware::isDebugging()
{
    return this == Cube::Debug::cube;
}


};  // namespace Cube
