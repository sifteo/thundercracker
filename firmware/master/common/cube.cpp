/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

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

    // First priority: Send video buffer updates.

    codec.encodeVRAM(tx.packet, vbuf);

    // Second priority: Download assets to flash

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
	    codec.flashReset(tx.packet)) {

	    // Remember that we're waiting for a reset ACK
	    Atomic::Or(flashResetSent, bit());

	    // Must end the packet after a flash escape.
	    return true;
	}

    } else {
	// Not waiting on a reset. See if we need to send asset data.

	bool done = false;
	_SYSAssetGroup *group = loadGroup;

	if (group && codec.flashSend(tx.packet, group, assetCube(group), done)) {
	    if (done) {
		/* Finished asset loading */
		Atomic::Or(group->doneCubes, bit());
		loadGroup = NULL;
	    }

	    // Must end the packet after a flash escape.
	    return true;
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
	    codec.flashAckBytes(loadACK);
	}
    }

    loadPrevACK = ack->flash_fifo_bytes;
    Atomic::Or(ackValid, bit());
}

void CubeSlot::radioTimeout()
{
    /* XXX: Disconnect this cube */
}
