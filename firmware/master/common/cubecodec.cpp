/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h> //XXX

#include <protocol.h>
#include <sifteo/machine.h>

#include "cubecodec.h"

using namespace Sifteo;
using namespace Sifteo::Intrinsic;


void CubeCodec::encodeVRAM(PacketBuffer &buf, _SYSVideoBuffer *vb)
{
    // This cube has no framebuffer at the moment, its rendering is disabled
    if (!vb)
	return;

    /*
     * Note that we have to sweep that change map as we go. Since
     * we're running in an ISR, at higher priority than user code,
     * it's possible for us to run between a vram modification and a
     * cm update (totally safe) but it isn't possible for any user
     * code to run during this ISR. So there's no need to worry about
     * atomicity in updating the cm here.
     *
     * Also note that we don't need to keep a separate pointer to
     * track our scan location in VRAM, since it's very cheap to use
     * our changemap to locate the first word in VRAM that needs
     * to be sent.
     *
     * Because of this, we effectively do a top-down scan of VRAM for
     * every RF packet. If something earlier on in the framebuffer was
     * changed by userspace, we'll go back and resend it before
     * sending later portions of the buffer. That's good in a way,
     * since it gives us a way to set a kind of QoS priority based on
     * VRAM address. But it means that userspace needs to consciously
     * delay writing to the changemap if it wants to avoid sending
     * redundant or out-of-order updates over the radio while a large
     * or multi-part update is in progress.
     *
     * This loop terminates when all of VRAM has been flushed, or when
     * we fill up the output packet. We assume that this function
     * begins with space available in the packet buffer.
     */
 
    do {
	/* This AND is important, it prevents out-of-bounds indexing
	 * or infinite looping if userspace gives us a bogus cm32
	 * value! This is equivalent to bounds-checking idx32 and addr
	 * below, as well as checking for infinite looping. Much more
	 * efficient to do it here though :)
	 */
	uint32_t cm32 = vb->cm32 & 0xFFFF0000;

	if (!cm32)
	    break;

	uint32_t idx32 = CLZ(cm32);
	uint32_t cm1 = vb->cm1[idx32];

	if (cm1) {
	    uint32_t idx1 = CLZ(cm1);
	    uint16_t addr = (idx32 << 5) | idx1;

	    /*
	     * XXX: This is allowed to abort encoding right now
	     *      because it's dumb and needs to output codezilla
	     *      and it may not fit in the packet buffer. But the
	     *      real encoder would never generate codes larger
	     *      than the txBits buffer, so we should always be
	     *      able to fully fill a packet buffer before
	     *      quitting.
	     */
	    if (!encodeVRAM(buf, addr, vb->words[addr]))
		return;

	    cm1 &= ROR(0x7FFFFFFF, idx1);
	    vb->cm1[idx32] = cm1;
	}

	if (!cm1) {
	    cm32 &= ROR(0x7FFFFFFF, idx32);
	    vb->cm32 = cm32;
	}
    } while (!buf.isFull());
}

bool CubeCodec::encodeVRAM(PacketBuffer &buf, uint16_t addr, uint16_t data)
{
    /*
     * XXX: Big honkin' literal codes! Debugging only... This takes a
     *      massive 5 bytes to encode one tile index!
     *
     * Note: Delta encoding is going to be kinda tricky. Since
     *       userspace can modify the VRAM buffer between ISRs, we can
     *       only reference deltas which haven't been modified. One
     *       way to do this would be with second CM that tracks which
     *       values we know have been flushed to the hardware
     *       successfully. The userspace code would be required to
     *       clear bits in that CM *before* touching them in VRAM.
     */

    printf("Encode %04x %04x\n", addr, data);
    
    if (!txBits.hasRoomForFlush(buf, 16 + 24))
	return false;

    txBits.append(3 | ((addr >> 4) & 0x10) | (addr & 0xFF) << 8, 16);
    txBits.flush(buf);

    txBits.append(0x23 | (data << 8), 24);
    txBits.flush(buf);

    return true;
}

bool CubeCodec::flashReset(PacketBuffer &buf)
{
    // No room in output buffer
    if (!txBits.hasRoomForFlush(buf, 12))
	return false;

    loadBufferAvail = FLS_FIFO_SIZE - 1;

    /*
     * Escape to flash mode (two-nybble code 33) plus one extra
     * dummy nybble to force a byte flush. Must be the last full
     * byte in the packet to trigger a reset.
     */
    txBits.append(0x333, 12);
    txBits.flush(buf);
    txBits.init();
    
    return true;
}

bool CubeCodec::flashSend(PacketBuffer &buf, _SYSAssetGroup *group,
			  _SYSAssetGroupCube *ac, bool &done)
{
    /*
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

    // Cube has no room in its buffer
    if (!loadBufferAvail)
	return false;

    // No room in output buffer
    if (!txBits.hasRoomForFlush(buf, 12 + 8))
	return false;

    // Per-cube asset state pointer is invalid
    if (!ac)
	return false;

    // Group data header pointer is invalid
    const _SYSAssetGroupHeader *ghdr = group->hdr;
    if (!Runtime::checkUserPointer(ghdr, sizeof *ghdr))
	return false;

    // Inconsistent sizes
    uint32_t dataSize = ghdr->dataSize;
    uint32_t progress = ac->progress;
    if (progress > dataSize)
	return false;
    
    uint8_t *src = (uint8_t *)ghdr + ghdr->hdrSize + progress;
    uint32_t count;

    // Flash escape code
    txBits.append(0x333, 12);
    txBits.flush(buf);
    txBits.init();

    // We're limited by the size of the packet, the asset, and the cube's FIFO
    count = MIN(buf.bytesFree(), dataSize - progress);
    count = MIN(count, loadBufferAvail);

    buf.appendUser(src, count);

    progress += count;
    loadBufferAvail -= count;
    ac->progress = progress;

    if (progress >= dataSize)
	done = true;

    return true;
}
