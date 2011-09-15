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
     * First priority: Continue an opcode we already started
     */

    if (rfOpcode != RF_OP_NOP) {
	txContinue(tx);
	if (tx.packet.isFull())
	    return true;
    }

    /*
     * Second priority: Begin a flash decoder reset
     *
     * We can only do this if a reset is needed, hasn't already been
     * sent, and we have a valid ACK (so there's a baseline for
     * checking whether the reset has been acknowledged.) Send the
     * reset token, and synchronously reset any flash-related IRQ
     * state.
     */

    if ((flashResetWait & bit()) && !(flashResetSent & bit()) && (ackValid & bit())) {
	Atomic::Or(flashResetSent, bit());
	loadBufferAvail = FLS_FIFO_SIZE - 1;

	tx.packet.append(RF_OP_FLASH_RESET);
	if (tx.packet.isFull())
	    return true;	   
    }

    /*
     * Third priority: Video buffer updates
     */

    if (vbuf->cm4) {
	/* Video buffer updates */
    }

    /*
     * Fourth priority: Asset downloading
     */

    _SYSAssetGroup *group = loadGroup;
    if (group && !(flashResetWait & bit())) {
	/*
	 * A note on security: Typically we have to be very paranoid
	 * about values obtained via the _SYS data structures, and we
	 * especially have to be careful when we're in an ISR. We have
	 * to read the values only once, then validate them before
	 * using.
	 *
	 * But here, nothing is really at stake if a buggy or
	 * malicious program gives us progress/dataSize values that
	 * are inconsistent. The worst that can happen is we generate
	 * a bogus length for the opcode below, and we send some
	 * garbage to the cube's flash decoder. Memory overruns and
	 * state consistency are rigidly enforced by txContinue().
	 */

	_SYSAssetGroup *group = loadGroup;
	const _SYSAssetGroupHeader *ghdr = group->hdr;
	_SYSAssetGroupCube *ac = assetCube(group);
	uint32_t dataSize = Runtime::checkUserPointer(ghdr, sizeof *ghdr) ? ghdr->dataSize : 0;
	uint32_t progress = ac ? ac->progress : 0;
	uint8_t packetSize;

	packetSize = MIN(RF_ARG_MASK + 1, dataSize - progress);
	packetSize = MIN(packetSize, loadBufferAvail);

	if (packetSize) {
	    rfOpcode = RF_OP_FLASH_QUEUE | (packetSize - 1);  
	    loadBufferAvail -= packetSize;

	    tx.packet.append(rfOpcode);
	    if (tx.packet.isFull())
		return true;	   
	
	    txContinue(tx);
	    if (tx.packet.isFull())
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

void CubeSlot::txContinue(PacketTransmission &tx)
{
    /*
     * Pick up where we left off, generating transmittable bytes for
     * the current RF opcode. This needs to handle any opcode that
     * takes arguments.
     */

    switch (rfOpcode & RF_OP_MASK) {

    case RF_OP_FLASH_QUEUE: {
	/*
	 * Continue writing Flash loadstream data.
	 *
	 * Since we're dealing with asset group pointers as well as
	 * per-cube state that reside in untrusted memory, this code
	 * has to be carefully written to read each value exactly
	 * once, and check it before use.
	 */

	_SYSAssetGroup *group = loadGroup;
	const _SYSAssetGroupHeader *ghdr = group->hdr;
	_SYSAssetGroupCube *ac = assetCube(group);
	bool ghdrValid = Runtime::checkUserPointer(ghdr, sizeof *ghdr);
	uint32_t dataSize = ghdrValid ? ghdr->dataSize : 0;
	uint32_t progress = ac ? ac->progress : 0;

	if (progress < dataSize) {
	    uint8_t *src = ghdrValid ? (uint8_t *)ghdr + ghdr->hdrSize + progress : 0;
	    uint32_t count;

	    count = MIN(tx.packet.bytesFree(), dataSize - progress);
	    count = MIN(count, (rfOpcode & RF_ARG_MASK) + 1);

	    tx.packet.appendUser(src, count);
	    progress += count;
	    rfOpcode -= count;

	    if (ac)
		ac->progress = progress;

	    if ((rfOpcode & RF_OP_MASK) != RF_OP_FLASH_QUEUE)
		rfOpcode = RF_OP_NOP;
	}

	if (progress >= dataSize) {
	    /* Finished asset loading */
	    Atomic::Or(group->doneCubes, bit());
	    loadGroup = NULL;
	}

	break;
    }

    }
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
