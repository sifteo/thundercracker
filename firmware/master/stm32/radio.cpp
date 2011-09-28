/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "radio.h"
#include "hardware.h"
#include "gpio.h"
#include "spi.h"

/*
 * Ports
 */

static GPIOPin RadioCE(&GPIOB, 10);
static SPIMaster RadioSPI(&SPI2,
			  GPIOPin(&GPIOB, 12),   // CSN
			  GPIOPin(&GPIOB, 13),   // SCK
			  GPIOPin(&GPIOB, 14),   // MISO
			  GPIOPin(&GPIOB, 15));  // MOSI

/*
 * SPI Commands
 */
#define CMD_R_REGISTER		0x00
#define CMD_W_REGISTER		0x20
#define CMD_R_RX_PAYLOAD    	0x61
#define CMD_W_TX_PAYLOAD	0xA0
#define CMD_FLUSH_TX		0xE1
#define CMD_FLUSH_RX		0xE2
#define CMD_REUSE_TX_PL		0xE3
#define CMD_R_RX_PL_WID		0x60
#define CMD_W_ACK_PAYLOAD	0xA8
#define CMD_W_TX_PAYLOAD_NO_ACK	0xB0
#define CMD_NOP			0xFF

/*
 * Registers
 */
#define REG_CONFIG		0x00
#define REG_EN_AA		0x01
#define REG_EN_RXADDR		0x02
#define REG_SETUP_AW		0x03
#define REG_SETUP_RETR		0x04
#define REG_RF_CH		0x05
#define REG_RF_SETUP		0x06
#define REG_STATUS		0x07
#define REG_OBSERVE_TX		0x08
#define REG_RPD			0x09
#define REG_RX_ADDR_P0		0x0A
#define REG_RX_ADDR_P1		0x0B
#define REG_RX_ADDR_P2		0x0C
#define REG_RX_ADDR_P3		0x0D
#define REG_RX_ADDR_P4		0x0E
#define REG_RX_ADDR_P5		0x0F
#define REG_TX_ADDR		0x10
#define REG_RX_PW_P0		0x11
#define REG_RX_PW_P1		0x12
#define REG_RX_PW_P2		0x13
#define REG_RX_PW_P3		0x14
#define REG_RX_PW_P4		0x15
#define REG_RX_PW_P5		0x16
#define REG_FIFO_STATUS		0x17
#define REG_DYNPD		0x1C
#define REG_FEATURE		0x1D


void Radio::open()
{
    RadioCE.setControl(GPIOPin::OUT_10MHZ);
    RadioSPI.init();

    while (1) {
	RadioSPI.begin();
	RadioSPI.transfer(CMD_W_REGISTER);
	RadioSPI.end();
    }
}

void Radio::halt()
{
}
