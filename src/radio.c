/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include "radio.h"

#define RX_INTERVAL_US       750
#define RX_INTERVAL_CYCLES   (opt_clock_hz * RX_INTERVAL_US / 1000 / 1000)
extern int opt_clock_hz;


struct {
    int rx_timer;
} radio;

void radio_init(void)
{
}

void radio_exit(void)
{
}

uint8_t radio_spi_byte(uint8_t mosi)
{
    return 0xFF;
}

void radio_ctrl(int csn, int ce)
{
}

int radio_tick(void)
{
    if (--radio.rx_timer <= 0) {
	/*
	 * Simulate the rate at which we can actually receive RX packets over the air,
	 * by giving ourselves a receive opportunity at fixed clock cycle intervals.
	 */

	uint8_t payload[256];

	network_rx(payload);
	radio.rx_timer = RX_INTERVAL_CYCLES;
    }	

    return 0;
}
