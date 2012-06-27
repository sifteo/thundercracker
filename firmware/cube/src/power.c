/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "power.h"
#include "cube_hardware.h"
#include "lcd.h"
#include "radio.h"
#include "sensors_i2c.h"

uint8_t power_sleep_timer;

void power_delay()
{
    // Arbitrary delay, currently about 12 ms.
    uint8_t delay_i = 0, delay_j;
    do {
        delay_j = 0;
        while (--delay_j);
    } while (--delay_i);
}


void power_init(void)
{
    /*
     * First, clean up after any possible sleep state we were just in. The
     * data sheet specifies that we need to rest PWRDWN to 0 after reading it,
     * and we need to re-open the I/O latch.
     *
     * We don't care why we were sleeping. If we did, though, we could do
     * something useful with the value read back from PWRDWN.
     */

    PWRDWN;
    OPMCON = 0;
    WUOPC1 = 0;
    PWRDWN = 0;

    /*
     * Basic poweron.
     *
     * Safe defaults, everything off.
     * all control lines must be low before supply rails are turned on.
     */

    MISC_PORT = 0;
    CTRL_PORT = 0;
    ADDR_PORT = 0;
    BUS_DIR = 0xFF;
    MISC_DIR = MISC_DIR_VALUE;
    CTRL_DIR = CTRL_DIR_VALUE;
    ADDR_DIR = 0;

    // Sequence 3.3v boost, followed by 2.0v downstream
    #if HWREV >= 2
        // Turn on 3.3V boost
        CTRL_PORT = CTRL_3V3_EN;

        // Give 3.3V boost >1ms to turn-on
        power_delay();

        // Turn on 2V ds load switch
        CTRL_PORT = CTRL_3V3_EN | CTRL_DS_EN;

        // Give load-switch time to turn-on (Datasheet unclear so >1ms should suffice)
        power_delay();
    #endif

    // Now turn-on other control lines.
    // (On Rev 1, we just turn everything on at once.)
    CTRL_PORT = CTRL_IDLE;
    MISC_PORT = MISC_IDLE;

    /*
     * By now, we should have a stable 16 MHz oscillator. Turn on
     * CLKLF, digitally synthesizing it from the 16 MHz crystal.
     *
     * Then, turn on the watchdog timer. This WDT timeout must be
     * long enough to cover the full initialization procedure, from
     * here until when we enter the main loop.
     */
    CLKLFCTRL = CLKLFCTRL_SRC_SYNTH;
    power_wdt_set();

    /*
     * Neighbor Tx Experimental setting.
     * Hope is to generate stronger magnetic field
     */
    #ifdef NBR_HIGH_DRIVE
        MISC_CON = 0x60;
        MISC_CON = 0x61;
        MISC_CON = 0x65;
        #if HWREV >= 1
            MISC_CON = 0x64;
        #else
            MISC_CON = 0x67;
        #endif
    #endif

    /*
     *  Neighbor Rx Experimental setting.
     *  Hope is to provide greater damping for tank oscillation
     */
    #ifdef NBR_PULLDOWN
        MISC_CON = 0x30;
        MISC_CON = 0x31;
        MISC_CON = 0x35;
        #if HWREV >= 1
            MISC_CON = 0x34;
        #else
            MISC_CON = 0x37;
        #endif
    #endif
}

void power_sleep(void)
{
    /*
     * Turn off all peripherals, and put the CPU into Deep Sleep mode.
     * Order matters, don't cause bus contention!
     */

#if HWREV >= 1   // Rev 1 was the first with sleep support

    lcd_sleep();                // Sleep sequence for LCD controller
    cli();                      // Stop all interrupt handlers
    radio_rx_disable();         // Take the radio out of RX mode

    RF_CKEN = 0;                // Stop the radio clock
    RNGCTL = 0;                 // RNG peripheral off
    ADCCON1 = 0;                // ADC peripheral off
    W2CON0 = 0;                 // I2C peripheral off
    S0CON = 0;                  // UART peripheral off
    SPIMCON0 = 0;               // External SPI disabled
    SPIRCON0 = 0;               // Radio SPI disabled

    BUS_DIR = 0xFF;             // Float the bus before we've set CTRL_PORT

    ADDR_PORT = 0;              // Address bus must be all zero
    MISC_PORT = 0;              // Neighbor/I2C set to idle-mode as well

#if HWREV >= 2
    // Sequencing is important. First, bring flash control lines low
    CTRL_PORT = CTRL_3V3_EN | CTRL_DS_EN;

    // Turn off the 2V DS rail
    CTRL_PORT = CTRL_3V3_EN;

    // Give the 2V DS rail some time to discharge (required?)
    // power_delay();

    // Turn off the 3.3V rail
    CTRL_PORT = 0;
#else
    // Turn the 3.3v boost and backlight off, leave WE/OE driven high (flash idle).
    CTRL_PORT = CTRL_FLASH_WE | CTRL_FLASH_OE;
#endif

    ADDR_DIR = 0;               // Drive address bus
    MISC_DIR = 0xFF;            // All MISC pins as inputs (I2C bus pulled up)
    //CTRL_DIR = CTRL_DIR_VALUE;  // All CTRL pins driven
    CTRL_DIR = CTRL_DIR_VALUE | CTRL_FLASH_LAT1;	//input on INT2 from acc
    
    BUS_PORT = 0;               // Drive bus port low
    BUS_DIR = 0;

#if HWREV >= 5
    /*
     * Wakeup on shake
     */

    W2CON0 = 0;                 // Reset I2C master
    W2CON0 = 1;                 // turn it on
    W2CON0 = 7;                 // Turn on I2C controller, Master mode, 100 kHz.

    // Enable inertial interrupt on INT2
    {
        static const __code uint8_t init[] = {
            3, ACCEL_ADDR_TX, ACCEL_CTRL_REG2, ACCEL_REG2_INIT,
            3, ACCEL_ADDR_TX, ACCEL_CTRL_REG6, ACCEL_REG6_INIT,
            3, ACCEL_ADDR_TX, ACCEL_INT1_CFG,  ACCEL_CFG_INIT,
            3, ACCEL_ADDR_TX, ACCEL_INT1_THS,  ACCEL_THS_INIT,
            3, ACCEL_ADDR_TX, ACCEL_INT1_DUR,  ACCEL_DUR_INIT,
            0
        };
        i2c_tx(init);
    }

    WUOPC1 = SHAKE_WUOPC_BIT;

#else
    /*
     * Wakeup on touch
     */

    WUOPC1 = TOUCH_WUOPC_BIT;
#endif

    // We must latch these port states, to preserve them during sleep.
    // This latch stays locked until early wakeup, in power_init().
    OPMCON = OPMCON_LATCH_LOCKED;
    // We must also disable WDT in the memory retention mode
    //OPMCON = OPMCON_LATCH_LOCKED | OPMCON_WDT_RESET_ENABLE;

    while (1) {
        PWRDWN = 1;
        //PWRDWN = 3;

        /*
         * We loop purely out of paranoia. This point should never be reached.
         * On wakeup, the system experiences a soft reset.
         */
    }

#endif  // HWREV >= 1
}
