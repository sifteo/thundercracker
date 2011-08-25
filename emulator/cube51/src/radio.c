/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Emulated a nRF24L01/nRF24LE1 radio.
 *
 * We only handle ShockBurst PRX mode with auto-ack and a single
 * receive pipe (P0).
 */

#include <stdio.h>
#include <string.h>
#include "radio.h"
#include "network.h"
#include "emulator.h"

#define RX_INTERVAL_US	750

/* SPI Commands */
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

/* Registers */
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

/* STATUS bits */
#define STATUS_TX_FULL		0x01
#define STATUS_RX_P_MASK	0X0E
#define STATUS_MAX_RT		0x10
#define STATUS_TX_DS		0x20
#define STATUS_RX_DR		0x40

/* FIFO_STATUS bits */
#define FIFO_RX_EMPTY     	0x01
#define FIFO_RX_FULL		0x02
#define FIFO_TX_EMPTY		0x10
#define FIFO_TX_FULL		0x20
#define FIFO_TX_REUSE		0x40

#define FIFO_SIZE		3
#define PAYLOAD_MAX		32

struct radio_packet {
    uint8_t len;
    uint8_t payload[PAYLOAD_MAX];
};

struct {
    int rx_timer;
    uint32_t byte_count;
    uint32_t rx_count;
    uint8_t irq_state;

    /*
     * Keep these consecutive, for the debugger's memory editor view.
     * It can currently view/edit 0x38 bytes, starting at regs[0].
     */
    union {
	uint8_t debug[0x38];
	struct {
	    uint8_t regs[0x20];		// 00 - 1F
	    uint8_t addr_tx_high[4];	// 20 - 23
	    uint8_t addr_rx0_high[4];	// 24 - 27
	    uint8_t addr_rx1_high[4];	// 28 - 2B
	    uint8_t rx_fifo_count;	// 2C
	    uint8_t tx_fifo_count;	// 2D
	    uint8_t rx_fifo_head;	// 2E
	    uint8_t rx_fifo_tail;	// 2F
	    uint8_t tx_fifo_head;	// 30
	    uint8_t tx_fifo_tail;	// 31
	    uint8_t csn;		// 32
	    uint8_t ce;			// 33
	    uint8_t spi_cmd;		// 34
	    int8_t spi_index;		// 35
	};
    };

    struct radio_packet rx_fifo[FIFO_SIZE];
    struct radio_packet tx_fifo[FIFO_SIZE];

    struct em8051 *cpu;	// Only for exception reporting!
} radio;

void radio_init(struct em8051 *cpu)
{
    memset(&radio, 0, sizeof radio);
    radio.cpu = cpu;

    radio.regs[REG_CONFIG] = 0x08;
    radio.regs[REG_EN_AA] = 0x3F;
    radio.regs[REG_EN_RXADDR] = 0x03;
    radio.regs[REG_SETUP_AW] = 0x03;
    radio.regs[REG_SETUP_RETR] = 0x03;
    radio.regs[REG_RF_CH] = 0x02;
    radio.regs[REG_RF_SETUP] = 0x0E;
    radio.regs[REG_STATUS] = 0x0E;
    radio.regs[REG_RX_ADDR_P0] = 0xE7;
    radio.regs[REG_RX_ADDR_P1] = 0xC2;
    radio.regs[REG_RX_ADDR_P2] = 0xC3;
    radio.regs[REG_RX_ADDR_P3] = 0xC4;
    radio.regs[REG_RX_ADDR_P4] = 0xC5;
    radio.regs[REG_RX_ADDR_P5] = 0xC6;
    radio.regs[REG_TX_ADDR] = 0xE7;
    radio.regs[REG_FIFO_STATUS] = 0x11;

    memset(radio.addr_tx_high, 0xE7, 4);
    memset(radio.addr_rx0_high, 0xE7, 4);
    memset(radio.addr_rx1_high, 0xC2, 4);
}

void radio_exit(void)
{
    /* Nothing to do yet */
}

uint8_t *radio_regs(void)
{
    return radio.regs;
}

uint32_t radio_rx_count(void)
{
    uint32_t c = radio.rx_count;
    radio.rx_count = 0;
    return c;
}

uint32_t radio_byte_count(void)
{
    uint32_t c = radio.byte_count;
    radio.byte_count = 0;
    return c;
}

static void radio_update_status(void)
{
    radio.regs[REG_FIFO_STATUS] =
	(radio.rx_fifo_count == 0 ? FIFO_RX_EMPTY : 0) |
	(radio.rx_fifo_count == FIFO_SIZE ? FIFO_RX_FULL : 0) |
	(radio.tx_fifo_count == 0 ? FIFO_TX_EMPTY : 0) |
	(radio.tx_fifo_count == FIFO_SIZE ? FIFO_TX_FULL : 0);

    radio.regs[REG_STATUS] &= STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT;

    if (radio.tx_fifo_count == FIFO_SIZE)
	radio.regs[REG_STATUS] &= STATUS_TX_FULL;

    if (radio.rx_fifo_count == 0)
	radio.regs[REG_STATUS] |= STATUS_RX_P_MASK;

    radio.regs[REG_RX_PW_P0] = radio.rx_fifo[radio.rx_fifo_tail].len;
}

static void radio_spi_cmd_begin(uint8_t cmd)
{
    /*
     * Handle commands that have an immediate side-effect, even before
     * we get data bytes.
     */
    switch (cmd) {

    case CMD_FLUSH_TX:
	radio.tx_fifo_count = 0;
	radio_update_status();
	break;

    case CMD_FLUSH_RX:
	radio.rx_fifo_count = 0;
	radio_update_status();
	break;

    }
}

static void radio_spi_cmd_end(uint8_t cmd)
{
    /*
     * End a command, invoked at the point where CS is deasserted.
     */
    switch (cmd) {

    case CMD_W_TX_PAYLOAD:
    case CMD_W_TX_PAYLOAD_NO_ACK:
    case CMD_W_ACK_PAYLOAD:
	radio.tx_fifo[radio.tx_fifo_head].len = radio.spi_index;
	if (radio.tx_fifo_count < FIFO_SIZE) {
	    radio.tx_fifo_count++;
	    radio.tx_fifo_head = (radio.tx_fifo_head + 1) % FIFO_SIZE;
	} else
	    radio.cpu->except(radio.cpu, EXCEPTION_RADIO_XRUN);
	break;

    case CMD_R_RX_PAYLOAD:
	if (radio.rx_fifo_count > 0) {
	    radio.rx_fifo_count--;
	    radio.rx_fifo_tail = (radio.rx_fifo_tail + 1) % FIFO_SIZE;
	} else
	    radio.cpu->except(radio.cpu, EXCEPTION_RADIO_XRUN);
	break;
    }
}

static uint8_t *radio_reg_ptr(uint8_t reg, unsigned byte_index)
{
    reg &= sizeof radio.regs - 1;

    if (byte_index > 4)
	byte_index = 4;

    if (byte_index > 0)
	switch (reg) {
	case REG_TX_ADDR:
	    return &radio.addr_tx_high[byte_index - 1];
	case REG_RX_ADDR_P0:
	    return &radio.addr_rx0_high[byte_index - 1];
	case REG_RX_ADDR_P1:
	    return &radio.addr_rx1_high[byte_index - 1];
	}
    
    return &radio.regs[reg];
}

static uint64_t radio_pack_addr(uint8_t reg)
{
    /*
     * Encode the nRF24L01 packet address plus channel in the
     * 64-bit address word used by our network message hub.
     */

    uint64_t addr = 0;
    int i;

    for (i = 4; i >= 0; i--)
	addr = (addr << 8) | *radio_reg_ptr(reg, i);

    return addr | ((uint64_t)radio.regs[REG_RF_CH] << 56);
}

static uint8_t radio_spi_cmd_data(uint8_t cmd, unsigned index, uint8_t mosi)
{
    switch (cmd) {

    case CMD_R_RX_PAYLOAD:
	return radio.rx_fifo[radio.rx_fifo_tail].payload[index % PAYLOAD_MAX];

    case CMD_W_TX_PAYLOAD:
    case CMD_W_TX_PAYLOAD_NO_ACK:
    case CMD_W_ACK_PAYLOAD:
	radio.tx_fifo[radio.tx_fifo_head].payload[index % PAYLOAD_MAX] = mosi;
	return 0xFF;

    case CMD_W_REGISTER | REG_STATUS:
	// Status has write-1-to-clear bits
	mosi &= STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT;
	*radio_reg_ptr(cmd, index) &= ~mosi;
	return 0xFF;

    default:
	if (cmd < CMD_R_REGISTER + sizeof radio.regs)
	    return *radio_reg_ptr(cmd, index);
	if (cmd < CMD_W_REGISTER + sizeof radio.regs) {
	    *radio_reg_ptr(cmd, index) = mosi;
	    return 0xFF;
	}
    }
    return 0xFF;
}

uint8_t radio_spi_byte(uint8_t mosi)
{
    // Chip not selected?
    if (!radio.csn)
	return 0xFF;

    if (radio.spi_index < 0) {
	radio.spi_cmd = mosi;
	radio_spi_cmd_begin(mosi);
	radio.spi_index++;
	return radio.regs[REG_STATUS];
    }

    return radio_spi_cmd_data(radio.spi_cmd, radio.spi_index++, mosi);
}

void radio_ctrl(int csn, int ce)
{
    if (csn && !radio.csn) {
	// Begin new SPI command
	radio.spi_index = -1;
    }

    if (!csn && radio.csn) {
	// End an SPI command
	radio_spi_cmd_end(radio.spi_cmd);
    }
    
    radio.csn = csn;
    radio.ce = ce;
}

static void radio_rx_opportunity(void)
{
    struct radio_packet *rx_head = &radio.rx_fifo[radio.rx_fifo_head];
    struct radio_packet *tx_tail = &radio.tx_fifo[radio.tx_fifo_tail];
    uint64_t src_addr;
    int len;
    uint8_t payload[256];    

    /* Network receive opportunity */
    network_set_addr(radio_pack_addr(REG_RX_ADDR_P0));
    len = network_rx(&src_addr, payload);
    if (len < 0 || len > sizeof rx_head->payload)
	return;

    if (radio.rx_fifo_count < FIFO_SIZE) {
	rx_head->len = len;
	memcpy(rx_head->payload, payload, len);

	radio.rx_fifo_head = (radio.rx_fifo_head + 1) % FIFO_SIZE;
	radio.rx_fifo_count++;
	radio.regs[REG_STATUS] |= STATUS_RX_DR;

	// Statistics for the debugger
	radio.rx_count++;
	radio.byte_count += len;

	if (radio.tx_fifo_count) {
	    // ACK with payload
	    radio.byte_count += tx_tail->len;
	    network_tx(src_addr, tx_tail->payload, tx_tail->len);
	    radio.tx_fifo_tail = (radio.tx_fifo_tail + 1) % FIFO_SIZE;
	    radio.tx_fifo_count--;
	    radio.regs[REG_STATUS] |= STATUS_TX_DS;
	} else {
	    // ACK without payload (empty TX fifo)
	    network_tx(src_addr, NULL, 0);
	}
    } else
	radio.cpu->except(radio.cpu, EXCEPTION_RADIO_XRUN);

    radio_update_status();
}

int radio_tick(void)
{
    uint8_t irq_prev = radio.irq_state;

    /*
     * Simulate the rate at which we can actually receive RX packets
     * over the air, by giving ourselves a receive opportunity at
     * fixed clock cycle intervals.
     */
    if (radio.ce && --radio.rx_timer <= 0) {
	radio.rx_timer = USEC_TO_CYCLES(RX_INTERVAL_US);
	radio_rx_opportunity();
    }

    radio.irq_state = radio.regs[REG_CONFIG] & radio.regs[REG_STATUS] &
	(STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT);
    return radio.irq_state && !irq_prev;
}
