/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This defines an interface layer for talking to our network hub via
 * TCP. We can set an address, transmit messages to another node by
 * address, or receive messages.
 */

#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdint.h>

void network_init(const char *host, const char *port);
void network_exit(void);

void network_set_addr(uint64_t addr);
void network_tx(uint64_t addr, void *payload, int len);
int network_rx(uint8_t payload[256]);

#endif
