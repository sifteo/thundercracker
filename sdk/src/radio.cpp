/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include <stdio.h>    // XXX
#include <stdlib.h>   // XXX
#include "radio.h"

/*
 * XXX: Hack for testing loadstream performance
 */

namespace Sifteo {

#include "ls.h"

const static uint8_t *ls_ptr = loadstream;
static uint32_t buf_space = 127;
static uint32_t ls_remaining = sizeof loadstream;
static uint8_t prev_ack;


void RadioManager::produce(PacketTransmission &tx)
{
    static RadioAddress addr = { 0x02, { 0xe7, 0xe7, 0xe7, 0xe7, 0xe7 }};
    uint32_t size = ls_remaining;
    
    tx.dest = &addr;

    if (size > 31) size = 31;
    if (size > buf_space) size = buf_space;

    if (size) {
	tx.packet.len = size + 1;
	tx.packet.bytes[0] = 0xC0 | (size - 1);
	memcpy(tx.packet.bytes + 1, ls_ptr, size);
	ls_ptr += size;
	ls_remaining -= size;
	buf_space -= size;
    } else {
	tx.packet.len = 0;
    }

    if (ls_remaining == 0 && buf_space == 127) {
	printf("Done\n");
	exit(0);
    }
}

void RadioManager::acknowledge(const PacketBuffer &packet)
{
    if (packet.len > 11) {
	uint8_t ack = packet.bytes[11];
	buf_space += (uint8_t)(ack - prev_ack);
	prev_ack = ack;
    }
}

void RadioManager::timeout()
{
}

void RadioManager::add(RadioEndpoint *ep)
{
}

void RadioManager::remove(RadioEndpoint *ep)
{
}

};  // namespace Sifteo
