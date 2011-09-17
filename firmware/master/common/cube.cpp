/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h> // XXX

#include <protocol.h>
#include <sifteo/machine.h>

#include "cube.h"

using namespace Sifteo;

CubeSlot CubeSlot::instances[_SYS_NUM_CUBE_SLOTS];
_SYSCubeIDVector CubeSlot::vecEnabled;
_SYSCubeIDVector CubeSlot::flashResetWait;
_SYSCubeIDVector CubeSlot::flashResetSent;
_SYSCubeIDVector CubeSlot::ackValid;


void CubeSlot::loadAssets(_SYSAssetGroup *a) {
    _SYSAssetGroupCube *ac = assetCube(a);

    if (!ac)
	return;
    if (isAssetGroupLoaded(a))
	return;

    // XXX: Pick a base address too!
    ac->progress = 0;

    // Start by resetting the flash decoder. This must happen before we set 'loadGroup'.
    Atomic::And(flashResetSent, ~bit());
    Atomic::Or(flashResetWait, bit());

    // Then start streaming asset data for this group
    a->reqCubes |= bit();
    loadGroup = a;
}

bool CubeSlot::radioProduce(PacketTransmission &tx)
{
    /*
     * XXX: Try to connect, if we aren't connected. And use a real address.
     *      For now I'm hardcoding the default address, since that's what
     *      the emulator will come up with.
     */
    static const RadioAddress addr = { 0x02, { 0xe7, 0xe7, 0xe7, 0xe7, 0xe7 }};
    tx.dest = &addr;
    tx.packet.len = 0;

    /*
     * First priority: Send video buffer updates
     */

    if (vbuf->cm4) {
	/* Video buffer updates */
    }

    /*
     * Second priority: Download assets to flash
     */

    if (flashResetWait & bit()) {

	/*
	 * We need to reset the flash decoder before we can send any data.
	 *
	 * We can only do this if a reset is needed, hasn't already
	 * been sent, and we have a valid ACK (so there's a baseline
	 * for checking whether the reset has been acknowledged.) Send
	 * the reset token, and synchronously reset any flash-related
	 * IRQ state.
	 */

	if (!(flashResetSent & bit()) &&
	    (ackValid & bit()) &&
	    txBits.hasRoomForFlush(tx.packet, 12)) {

	    Atomic::Or(flashResetSent, bit());
	    loadBufferAvail = FLS_FIFO_SIZE - 1;

	    /*
	     * Escape to flash mode (two-nybble code 33) plus one extra
	     * dummy nybble to force a byte flush. Must be the last full
	     * byte in the packet to trigger a reset.
	     */
	    txBits.append(0x333, 12);
	    txBits.flush(tx.packet);
	    txBits.init();

	    // Must end the packet after a flash escape.
	    return true;
	}

    } else {

	/*
	 * Not waiting on a reset. See if we need to send asset data.
	 *
	 * Since we're dealing with asset group pointers as well as
	 * per-cube state that reside in untrusted memory, this code
	 * has to be carefully written to read each user value exactly
	 * once, and check it before use.
	 *
	 * We only do this if we have asset data, obviously, but also
	 * if the cube has enough buffer space to accept it, and if
	 * there's enough room in the packet for both the escape code
	 * and at least one byte of flash data.
	 *
	 * After this initial check, any further checks exist only as
	 * protection against buggy or malicious user code.
	 */

	_SYSAssetGroup *group = loadGroup;

	if (group && loadBufferAvail &&
	    txBits.hasRoomForFlush(tx.packet, 12 + 8)) {

	    const _SYSAssetGroupHeader *ghdr = group->hdr;
	    _SYSAssetGroupCube *ac = assetCube(group);

	    if (ac && Runtime::checkUserPointer(ghdr, sizeof *ghdr)) {
		uint32_t dataSize = ghdr->dataSize;
		uint32_t progress = ac->progress;

		if (progress < dataSize) {
		    uint8_t *src = (uint8_t *)ghdr + ghdr->hdrSize + progress;
		    uint32_t count;
		
		    txBits.append(0x333, 12);
		    txBits.flush(tx.packet);
		    txBits.init();

		    count = MIN(tx.packet.bytesFree(), dataSize - progress);
		    count = MIN(count, loadBufferAvail);

		    tx.packet.appendUser(src, count);

		    progress += count;
		    loadBufferAvail -= count;
		    ac->progress = progress;

		    if (progress >= dataSize) {
			/* Finished asset loading */
			Atomic::Or(group->doneCubes, bit());
			loadGroup = NULL;
		    }

		    // Must end the packet after a flash escape.
		    return true;
		}
	    }
	}
    }

    /*
     * XXX: We don't have to always return true... we can return false if
     *      we have no useful work to do, so long as we still occasionally
     *      return true to request a ping packet at some particular interval.
     */
    return true;
}

void CubeSlot::radioAcknowledge(const PacketBuffer &packet)
{
    RF_ACKType *ack = (RF_ACKType *) packet.bytes;

    if (packet.len < sizeof *ack)
	return;

    if (ackValid & bit()) {
	// Two valid ACK packets in a row.

	uint8_t loadACK = ack->flash_fifo_bytes - loadPrevACK;

	if (flashResetWait & bit()) {
	    // We're waiting on a reset
	    if (loadACK)
		Atomic::And(flashResetWait, ~bit());
	} else {
	    // Acknowledge FIFO bytes
	    loadBufferAvail += loadACK;
	}
    }

    loadPrevACK = ack->flash_fifo_bytes;
    Atomic::Or(ackValid, bit());
}

void CubeSlot::radioTimeout()
{
    /* XXX: Disconnect this cube */
}
