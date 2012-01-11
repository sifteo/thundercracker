/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cube_hardware.h"
#include "cube_debug.h"
#include "cube_cpu_callbacks.h"

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
    exceptionCount = 0;
    
    cpu.callbackData = this;
    cpu.vtime = masterTimer;
    cpu.mProfileData = NULL;
    
    CPU::em8051_reset(&cpu, true);

    if (firmwareFile) {
        if (CPU::em8051_load(&cpu, firmwareFile)) {
            fprintf(stderr, "Error: Failed to load firmware '%s'\n", firmwareFile);
            return false;
        }
    } else {
        CPU::em8051_init_sbt(&cpu);
    }

 
    if (!flashStorage.init(flashFile)) {
        fprintf(stderr, "Error: Failed to initialize flash memory\n");
        return false;
    }
    
    flash.init(&flashStorage);
    spi.radio.init(&cpu);
    spi.init(&cpu);
    adc.init();
    mdu.init();
    i2c.init();
    lcd.init();
    rng.init();
    neighbors.init();
    
    setTouch(0.0f);
    
    // XXX: Simulated battery level
    adc.setInput(0, 0x8760);
    
    return true;
}

void Hardware::reset()
{
    CPU::em8051_reset(&cpu, false);
}

void Hardware::exit()
{
    flashStorage.exit();
}

// cube_cpu_callbacks.h
void CPU::except(CPU::em8051 *cpu, int exc)
{
    Hardware *self = (Hardware*) cpu->callbackData;
    const char *name = CPU::em8051_exc_name(exc);
    
    self->incExceptionCount();

    Tracer::log(cpu, "@%04x EXCEPTION: %s", cpu->mPC, name);

    if (self == Cube::Debug::cube && Cube::Debug::stopOnException)
        Cube::Debug::emu_exception(cpu, exc);
    else
        fprintf(stderr, "[%2d] EXCEPTION at 0x%04x: %s\n", cpu->id, cpu->mPC, name);
}

// cube_cpu_callbacks.h
int CPU::NVM::write(CPU::em8051 *cpu, uint16_t addr, uint8_t data)
{
    Hardware *self = (Hardware*) cpu->callbackData;

    if (!(cpu->mSFR[REG_FSR] & (1<<5))) {
        // Write disabled
        except(cpu, EXCEPTION_NVM);
        return 0;
    }
            
    // Program flash bits (1 -> 0)
    self->flashStorage.data.nvm[addr] &= data;
    self->flashStorage.asyncWrite(*self->time, &self->cpu);
    
    // Self-timed write cycles
    return 12800;
}

// cube_cpu_callbacks.h
uint8_t CPU::NVM::read(CPU::em8051 *cpu, uint16_t addr)
{
    Hardware *self = (Hardware*) cpu->callbackData;
    return self->flashStorage.data.nvm[addr];
}

void Hardware::sfrWrite(int reg)
{
    CPU::SFR::writeInline(this, &cpu, reg);
}

int Hardware::sfrRead(int reg)
{
    return CPU::SFR::readInline(this, &cpu, reg);
}

void Hardware::debugByte()
{
     printf("DEBUG[%d]: %02x\n", cpu.id, cpu.mSFR[REG_DEBUG]);
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
        CPU::except(&cpu, CPU::EXCEPTION_BUS_CONTENTION);
    }
    
    flash_drv = flashp.data_drv;  
    cpu.mSFR[BUS_PORT] = bus;
}

void Hardware::setAcceleration(float xG, float yG, float zG)
{
    /*
     * Set the cube's current acceleration, in G's. Scale it
     * according to the accelerometer's maximum range (assuming
     * the firmware has it configured for a full scale of +/- 2g).
     */

    i2c.accel.setVector(scaleAccelAxis(xG), scaleAccelAxis(yG), scaleAccelAxis(zG));
}

NEVER_INLINE void Hardware::hwDeadlineWork() 
{
    cpu.needHardwareTick = false;
    hwDeadline.reset();

    lcd.tick(hwDeadline, &cpu);
    adc.tick(hwDeadline, &cpu);
    spi.tick(hwDeadline, cpu.mSFR + REG_SPIRCON0, &cpu);
    i2c.tick(hwDeadline, &cpu);
    flash.tick(hwDeadline, &cpu);
    flashStorage.tick(hwDeadline);
    spi.radio.tick(rfcken, &cpu);
}

void Hardware::setTouch(float amount)
{
    /*
     * The A/D converter measures the remaining charge on Chold after some
     * charge is transferred to the touch plate. So, lower values mean higher
     * capacitance. The scaling here is a really rough estimate based on Hakim's
     * bench tests so far.
     *
     * Note taht these are 16-bit full-scale values we're passing to the ADC
     * module. It truncates them and justifies them according to the ADC configuration.
     */

    adc.setInput(12, 1600 - 320 * amount);
}

bool Hardware::isDebugging()
{
    return this == Cube::Debug::cube;
}

uint32_t Hardware::getExceptionCount()
{
    return exceptionCount;
}

void Hardware::incExceptionCount()
{
    exceptionCount++;
}

void Hardware::initVCD(VCDWriter &vcd)
{
    /*
     * Set up trace variables for our VCD file output.
     *
     * This feature acts like a built-in logic analyzer;
     * during initialization we identify interesting variables,
     * then they're monitored during each clock tick (when tracing
     * is enabled) and the results are written to an industry
     * standard VCD file.
     */
     
    vcd.enterScope("gpio"); {
    
        // Parallel busses
        vcd.define("addr", &cpu.mSFR[ADDR_PORT], 8); 
        vcd.define("addr_dir", &cpu.mSFR[ADDR_PORT_DIR], 8); 
        vcd.define("bus", &cpu.mSFR[BUS_PORT], 8); 
        vcd.define("bus_dir", &cpu.mSFR[BUS_PORT_DIR], 8); 
    
        // Ctrl port, broken out
        vcd.define("lcd_dcx", &cpu.mSFR[CTRL_PORT], 1, 0);
        vcd.define("flash_lat2", &cpu.mSFR[CTRL_PORT], 1, 1);
        vcd.define("flash_lat1", &cpu.mSFR[CTRL_PORT], 1, 2);
        vcd.define("en3v3", &cpu.mSFR[CTRL_PORT], 1, 3);
        vcd.define("lcd_backlight", &cpu.mSFR[CTRL_PORT], 1, 4);
        vcd.define("flash_we", &cpu.mSFR[CTRL_PORT], 1, 5);
        vcd.define("flash_oe", &cpu.mSFR[CTRL_PORT], 1, 6);
        vcd.define("ctrl_dir", &cpu.mSFR[CTRL_PORT_DIR], 8); 

        // Misc port, broken out
        vcd.define("nb_top", &cpu.mSFR[MISC_PORT], 1, 0);
        vcd.define("nb_top_dir", &cpu.mSFR[MISC_PORT_DIR], 1, 0);
        vcd.define("nb_left", &cpu.mSFR[MISC_PORT], 1, 1);
        vcd.define("nb_left_dir", &cpu.mSFR[MISC_PORT_DIR], 1, 1);
        vcd.define("nb_bottom", &cpu.mSFR[MISC_PORT], 1, 7);
        vcd.define("nb_bottom_dir", &cpu.mSFR[MISC_PORT_DIR], 1, 7);
        vcd.define("nb_right", &cpu.mSFR[MISC_PORT], 1, 5);
        vcd.define("nb_right_dir", &cpu.mSFR[MISC_PORT_DIR], 1, 5);
        
        /*
         * Neighbor IN is sampled from t012 instead of MISC_PORT,
         * since the corresponding bit in MISC_PORT is cleared by
         * Neighbors::clearNeighborInput before we can read it.
         */
        vcd.define("nb_in", &cpu.t012, 1, 6);
        vcd.define("nb_in_dir", &cpu.mSFR[MISC_PORT_DIR], 1, 6);

    } vcd.leaveScope();
    
    vcd.enterScope("cpu"); {
        // Internal state
        vcd.define("irq_count", &cpu.irq_count, 3);
        vcd.define("PC", &cpu.mPC, 16);

        // Important registers
        vcd.define("TL0", &cpu.mSFR[REG_TL0], 8); 
        vcd.define("TH0", &cpu.mSFR[REG_TH0], 8); 
        vcd.define("TL1", &cpu.mSFR[REG_TL1], 8); 
        vcd.define("TH1", &cpu.mSFR[REG_TH1], 8); 
        vcd.define("TL2", &cpu.mSFR[REG_TL2], 8); 
        vcd.define("TH2", &cpu.mSFR[REG_TH2], 8); 
        vcd.define("TCON", &cpu.mSFR[REG_TCON], 8); 
        vcd.define("IRCON", &cpu.mSFR[REG_IRCON], 8); 
    } vcd.leaveScope();
}


};  // namespace Cube
