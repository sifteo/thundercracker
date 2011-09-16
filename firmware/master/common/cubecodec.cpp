/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <protocol.h>
#include <sifteo/machine.h>

#include "cubecodec.h"

using namespace Sifteo;


void CubeCodec::encodeVRAM(PacketBuffer &buf, _SYSVideoBuffer *vb)
{
    // This cube has no framebuffer at the moment, its rendering is disabled
    if (!vb)
	return;

    // Nothing changed in the framebufer
    if (!vb->cm16)
	return;


    /*
     * Note that we have to sweep that change map as we go. Since
     * we're running in an ISR, at higher priority than user code,
     * it's possible for us to run between a vram modification and a
     * cm update (totally safe) but it isn't possible for any user
     * code to run during this ISR. So there's no need to worry about
     * atomicity in updating the cm here.
     *
     * So we take a simple strategy here. Every corresponding bit in
     * cm1 is cleared as soon as we buffer a compression code that
     * accounts for a single word. At every 16-word boundary, we check
     * the prior 16 change bits in cm1. If they're all still clear, we
     * clear a bit in cm16.
     *
     * This accounts for the fact that userspace may (and probably
     * will) continue to write to VRAM between packets. We'll catch
     * any of these new writes on the next iteration through the
     * buffer. We never seek our VRAM pointer backwards until reaching
     * the end of the buffer.
     */

    do {
	if (testCM16(vb, scanPtr)) {
	    // Something exists in this 16-word block
	    
	    

	}
    } while (!buf.isFull() && vb->cm16);


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
