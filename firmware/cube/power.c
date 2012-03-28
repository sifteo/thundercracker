/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "power.h"
#include "hardware.h"
#include "lcd.h"
#include "radio.h"

uint8_t power_sleep_timer;


void power_init(void)
{
    /*
     * If we're being powered on without a wakup-from-pin, go directly to
     * sleep. We don't wake up when the batteries are first inserted, only
     * when we get a touch signal.
     *
     * This default can be overridden by compiling with -DWAKE_ON_POWERUP.
     */

    /*
     * First, clean up after any possible sleep state we were just in. The
     * data sheet specifies that we need to rest PWRDWN to 0 after reading it,
     * and we need to re-open the I/O latch.
     */
    uint8_t powerupReason = PWRDWN;
#if HWREV >= 2
    uint8_t delay_i, delay_j;
#endif
    OPMCON = 0;
    TOUCH_WUPOC = 0;
    PWRDWN = 0;

#ifdef SLEEP_ON_POWERUP
    if (!powerupReason)
        power_sleep();
#endif

    /*
     * Basic poweron
     */

    // Safe defaults, everything off.
    // all control lines must be low before supply rails are turned on.
    MISC_PORT = 0;
    CTRL_PORT = 0;
    ADDR_PORT = 0;
    BUS_DIR = 0xFF;
    MISC_DIR = MISC_DIR_VALUE;
    CTRL_DIR = CTRL_DIR_VALUE;
    ADDR_DIR = 0;

#if HWREV >= 2
    // Sequence 3.3v boost, followed by 2.0v downstream

    //Turn on 3.3V boost
    CTRL_PORT = CTRL_3V3_EN;
    //give 3.3V boost >1ms to turn-on
    delay_i=255;
    while(delay_i--) {
    	delay_j=255;
    	while(delay_j--);
    }

    //Turn on 2V ds load switch
    CTRL_PORT = CTRL_3V3_EN | CTRL_DS_EN;
    //give load-switch time to turn-on (Datasheet unclear so >1ms should suffice)
    delay_i=255;
    while(delay_i--) {
    	delay_j=255;
    	while(delay_j--);
    }

    //Now turn-on other control lines.
    CTRL_PORT = CTRL_IDLE;
    MISC_PORT = MISC_IDLE;
#else
    // Turn everything on at once.
    CTRL_PORT = CTRL_IDLE;
    MISC_PORT = MISC_IDLE;
#endif

    /*
     * Neighbor TX pins
     *
     * We enable pull-downs for input mode, when we're receiving pulses from
     * our neighbors. This improves the isolation between each side's input.
     *
     * High drive is enabled.
     */

    MISC_CON = 0x60;
    MISC_CON = 0x61;
    MISC_CON = 0x65;
#if HWREV >= 1
    MISC_CON = 0x64;
#else
    MISC_CON = 0x67;
#endif
}

void power_sleep(void)
{
    /*
     * Turn off all peripherals, and put the CPU into Deep Sleep mode.
     * Order matters, don't cause bus contention!
     */
#if HWREV >= 2
	uint8_t delay_i, delay_j;
#endif

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
    MISC_PORT = 0;      		// Neighbor/I2C set to idle-mode as well

#if HWREV >= 2
    //Sequencing is important
    // Bring flash control lines low
    CTRL_PORT = CTRL_3V3_EN | CTRL_DS_EN;

    // Turn off the 2V DS rail
    CTRL_PORT = CTRL_3V3_EN;
    //Give the 2V DS rail some time to discharge
    delay_i=255;
    while(delay_i--) {
    	delay_j=255;
       	while(delay_j--);
    }

    //Turn off the 3.3V rail
    CTRL_PORT = 0;
#else
    // Turn the 3.3v boost and backlight off, leave WE/OE driven high (flash idle).
    CTRL_PORT = CTRL_FLASH_WE | CTRL_FLASH_OE;
#endif

    ADDR_DIR = 0;               // Drive address bus
    MISC_DIR = 0xFF;            // All MISC pins as inputs (I2C bus pulled up)
    CTRL_DIR = CTRL_DIR_VALUE;  // All CTRL pins driven
    
    BUS_PORT = 0;               // Drive bus port low
    BUS_DIR = 0;

    /*
     * Wakeup on touch
     */

    TOUCH_WUPOC = TOUCH_WUOPC_BIT;

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

#endif  // HWREV >= 1
}
