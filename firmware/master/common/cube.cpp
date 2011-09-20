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
_SYSCubeIDVector CubeSlot::flashACKValid;


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
	 * been sent. Send the reset token, and synchronously reset
	 * any flash-related IRQ state.
	 *
	 * Note the flash reset's dual purpose, of both resetting the
	 * cube's flash state machine and triggering the cube to send
	 * us an ACK packet with a valid flash byte count. So, we
	 * actually end up sending two resets if we haven't yet seen a
	 * valid flash ACK from this cube.
	 */

	if (!(flashResetSent & bit()) && codec.flashReset(tx.packet)) {
	    // Remember that we're waiting for a reset ACK
	    Atomic::Or(flashResetSent, bit());
	}

    } else {
	// Not waiting on a reset. See if we need to send asset data.

	bool done = false;
	_SYSAssetGroup *group = loadGroup;

	if (group && !(group->doneCubes & bit()) &&
	    codec.flashSend(tx.packet, group, assetCube(group), done)) {

	    if (done) {
		/* Finished asset loading */
		Atomic::Or(group->doneCubes, bit());
		Atomic::Or(Event::assetDoneCubes, bit());
		Event::setPending(Event::ASSET_DONE);
	    }
	}
    }

    // Finalize this packet. Must be last.
    codec.endPacket(tx.packet);

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

    if (packet.len >= offsetof(RF_ACKType, flash_fifo_bytes) + sizeof ack->flash_fifo_bytes) {
	// This ACK includes a valid flash_fifo_bytes counter

	if (flashACKValid & bit()) {
	    // Two valid ACKs in a row, we can count bytes.

	    uint8_t loadACK = ack->flash_fifo_bytes - flashPrevACK;

	    if (flashResetWait & bit()) {
		// We're waiting on a reset
		if (loadACK)
		    Atomic::And(flashResetWait, ~bit());
	    } else {
		// Acknowledge FIFO bytes
		codec.flashAckBytes(loadACK);
	    }

	} else {
	    // Now we've seen one ACK. If we're doing a reset, send the token again.
	   
	    Atomic::Or(flashACKValid, bit());
	    Atomic::And(flashResetSent, ~bit());	    
	}	

	flashPrevACK = ack->flash_fifo_bytes;
    }

    if (packet.len >= offsetof(RF_ACKType, accel) + sizeof ack->accel) {
	// Has valid accelerometer data. Is it different from our previous state?

	int8_t x = ack->accel[0] - 0x80;
	int8_t y = ack->accel[1] - 0x80;

	if (x != accelState.x || y != accelState.y) {
	    accelState.x = x;
	    accelState.y = y;
	    Atomic::Or(Event::accelChangeCubes, bit());
	    Event::setPending(Event::ACCEL_CHANGE);
	}
    }
}

void CubeSlot::radioTimeout()
{
    /* XXX: Disconnect this cube */
}
