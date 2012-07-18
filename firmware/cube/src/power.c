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


void power_delay()
{
    /*
     * Arbitrary delay, currently about 2 ms.
     *
     * This needs to be long enough for the boost and the load switch
     * to turn on, but keeping the delay to a minimum will ensure
     * that we limit the duration of any flash-of-white-LCD after coming
     * back from a brownout reset. See below.
     */

    uint8_t delay_i = 43, delay_j;
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
     *
     * Runs before clearing RAM, but after initializing the radio.
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
     *
     * We need to sequence this carefully, so that we can bring up
     * the 3.3v and 2.0v rails without causing any contention.
     */

    MISC_PORT = 0;
    CTRL_PORT = 0;
    ADDR_PORT = 0;
    BUS_DIR = 0xFF;
    MISC_DIR = MISC_DIR_VALUE;
    CTRL_DIR = CTRL_DIR_VALUE;
    ADDR_DIR = 0;

    // Turn on 3.3V boost
    CTRL_PORT = CTRL_3V3_EN;

    // Give 3.3V boost >1ms to turn-on
    power_delay();

    // Turn on 2V ds load switch
    CTRL_PORT = CTRL_3V3_EN | CTRL_DS_EN;

    // Give load-switch time to turn-on (Datasheet unclear so >1ms should suffice)
    power_delay();

    /*
     * Now turn-on other control lines. Sequence them so that we're sure to latch
     * a value of zero for BLE and RST, so that the LCD powers up in reset and with
     * the backlight off.
     *
     * NB: There can still be a brief flash due to the delays above! If we happen
     *     to reboot while there's still some power on the DS rail, the latch's
     *     default state can be HIGH. I don't think we can safely latch a LOW
     *     state earlier than this, though, without risk of causing a latchup
     *     by driving LAT1/LAT2 above the DS rail voltage.
     */
    CTRL_PORT = CTRL_IDLE & ~CTRL_LCD_DCX;
    CTRL_PORT = (CTRL_IDLE & ~CTRL_LCD_DCX) | CTRL_FLASH_LAT1 | CTRL_FLASH_LAT2;
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
        MISC_CON = 0x64;
    #endif

    /*
     *  Neighbor Rx Experimental setting.
     *  Hope is to provide greater damping for tank oscillation
     */
    #ifdef NBR_PULLDOWN
        MISC_CON = 0x30;
        MISC_CON = 0x31;
        MISC_CON = 0x35;
        MISC_CON = 0x34;
    #endif
}

void power_sleep(void)
{
    /*
     * Turn off all peripherals, and put the CPU into Deep Sleep mode.
     * Order matters, don't cause bus contention!
     */

    lcd_sleep();                // Sleep sequence for LCD controller
    cli();                      // Stop all interrupt handlers
    radio_rx_disable();         // Take the radio out of RX mode

    RTC2CON = 0;                // Turn off RTC2 timer and interrupts
    RF_CKEN = 0;                // Stop the radio clock
    RNGCTL = 0;                 // RNG peripheral off
    ADCCON1 = 0;                // ADC peripheral off
    S0CON = 0;                  // UART peripheral off
    SPIMCON0 = 0;               // External SPI disabled
    SPIRCON0 = 0;               // Radio SPI disabled

    BUS_DIR = 0xFF;             // Float the bus before we've set CTRL_PORT

    ADDR_PORT = 0;              // Address bus must be all zero
    MISC_PORT = 0;              // Neighbor/I2C set to idle-mode as well

    // Sequencing is important. First, bring flash control lines low
    CTRL_PORT = CTRL_3V3_EN | CTRL_DS_EN;

    // Turn off the 2V DS rail
    CTRL_PORT = CTRL_3V3_EN;

    // Turn off the 3.3V rail
    CTRL_PORT = 0;

#if HWREV >= 5
    /*
     * Wakeup on shake
     */

    {
        static const __code uint8_t init[] = {
            // ADC turned off
            ACCEL_TEMP_CFG_REG, 0,

            // Enable inertial interrupt on INT2
            ACCEL_CTRL_REG2, ACCEL_REG2_INIT,
            ACCEL_CTRL_REG6, ACCEL_REG6_INIT,
            ACCEL_INT1_CFG,  ACCEL_CFG_INIT,
            ACCEL_INT1_THS,  ACCEL_THS_INIT,
            ACCEL_INT1_DUR,  ACCEL_DUR_INIT,

            0
        };

        i2c_accel_tx(init);
    
        // I2C controller off
        W2CON0 = 0;
        __asm I2C_PULSE() __endasm;
    }

    ADDR_DIR = 0xff;            // Set addr pins to inputs
    CTRL_DIR = 0xff;            // Set ctrl pins to inputs
    MISC_DIR = 0xff;             // Set bus pins to inputs
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

    while (1) {
        PWRDWN = 1;

        /*
         * We loop purely out of paranoia. This point should never be reached.
         * On wakeup, the system experiences a soft reset.
         */
    }
}
