/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware -- RF Test
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "cube_hardware.h"

#define radio_enable()      { RF_CE = 1;  }
#define radio_disable()     { RF_CE = 0;  }
#define spi_begin()         { RF_CSN = 0; }
#define spi_end()           { RF_CSN = 1; }

static void delay()
{
    // Arbitrary delay, currently about 12 ms.
    uint8_t delay_i = 0, delay_j;
    do {
        delay_j = 0;
        while (--delay_j);
    } while (--delay_i);
}

static void spi_byte(uint8_t b)
{
    SPIRDAT = b;
    while (!(SPIRSTAT & SPI_RX_READY));
    SPIRDAT;
}

static void radio_init()
{
    radio_disable();                 // Receiver starts out disabled
    RF_CKEN = 1;                     // Radio clock running

    /* Enable nRF24L01 features */
    spi_begin();
    spi_byte(RF_CMD_W_REGISTER | RF_REG_FEATURE);
    spi_byte(0x07);
    spi_end();

#ifdef PRX_MODE
    /* 2 Mbit, max transmit power */
    spi_begin();
    spi_byte(RF_CMD_W_REGISTER | RF_REG_RF_SETUP);
    spi_byte(0x0e);
    spi_end();

    /* Power up, PRX mode, Mask interrupts */
    spi_begin();
    spi_byte(RF_CMD_W_REGISTER | RF_REG_CONFIG);
    spi_byte(0x7f);
    spi_end();
#else
    /* 2 Mbit, max transmit power, continuous wave mode, pll lock */
    spi_begin();
    spi_byte(RF_CMD_W_REGISTER | RF_REG_RF_SETUP);
    spi_byte(0x9e);
    spi_end();

    /* Power up, PTX mode */
    spi_begin();
    spi_byte(RF_CMD_W_REGISTER | RF_REG_CONFIG);
    spi_byte(0x0e);
    spi_end();
#endif
}

static void radio_receive()
{
	while(1);
}

static void radio_transmit(uint8_t channel)
{
    spi_begin();
    spi_byte(RF_CMD_W_REGISTER | RF_REG_RF_CH);
    spi_byte(channel++);
    spi_end();

    radio_enable();
    while (1);
}

static void power_init(void)
{
    // Safe defaults, everything off.
    // all control lines must be low before supply rails are turned on.

    MISC_PORT = 0;
    CTRL_PORT = 0;
    ADDR_PORT = 0;
    BUS_DIR = 0xFF;
    MISC_DIR = MISC_DIR_VALUE;
    CTRL_DIR = CTRL_DIR_VALUE;
    ADDR_DIR = 0;

#if HWREV >= 2      // Sequence 3.3v boost, followed by 2.0v downstream
    // Turn on 3.3V boost
    CTRL_PORT = CTRL_3V3_EN;

    // Give 3.3V boost >1ms to turn-on
    delay();

    // Turn on 2V ds load switch
    CTRL_PORT = CTRL_3V3_EN | CTRL_DS_EN;

    // Give load-switch time to turn-on (Datasheet unclear so >1ms should suffice)
    delay();
#endif

    // Now turn-on other control lines.
    // (On Rev 1, we just turn everything on at once.)
    CTRL_PORT = CTRL_IDLE;
    MISC_PORT = MISC_IDLE;
}

void main(void)
{
    power_init();
    radio_init();
#ifdef PRX_MODE
    /* Radio in PRX mode */
    radio_receive();
#else
    /* Radio in PTX mode */
    radio_transmit(PTX_CHAN);
#endif
}
