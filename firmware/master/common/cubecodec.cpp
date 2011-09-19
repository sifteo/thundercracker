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
using namespace Sifteo::Intrinsic;


void CubeCodec::encodeVRAM(PacketBuffer &buf, _SYSVideoBuffer *vb)
{
    // Flush any lingering bits from before.
    txBits.flush(buf);

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

	    if (!encodeVRAM(buf, addr, vb->words[addr], vb)) {
		/*
		 * We ran out of room to encode. This should be rare,
		 * happening only when we're near the end of the
		 * packet buffer AND we're encoding a very large code,
		 * like a literal 16-bit write plus a literal address
		 * change.
		 */
		break;
	    }

	    cm1 &= ROR(0x7FFFFFFF, idx1);
	    vb->cm1[idx32] = cm1;
	}

	if (!cm1) {
	    cm32 &= ROR(0x7FFFFFFF, idx32);
	    vb->cm32 = cm32;
	}
    } while (!buf.isFull());
}

bool CubeCodec::encodeVRAM(PacketBuffer &buf, uint16_t addr, uint16_t data,
			   _SYSVideoBuffer *vb)
{
    /*
     * Address coding
     */

    if (addr != codePtr) {
	uint16_t delta = (addr - codePtr) & PTR_MASK;

	if (delta <= 8) {
	    // We can use a short skip code

	    delta--;
	    flushDSRuns();
	    txBits.append((delta & 1) | ((delta << 3) & 0x30), 8);
	    txBits.flush(buf);

	} else {
	    // Too large a delta, use a longer literal code

	    flushDSRuns();
	    txBits.append(3 | ((addr >> 4) & 0x10) | (addr & 0xFF) << 8, 16);
	    txBits.flush(buf);
	}

	codePtr = addr;
    }

    if (buf.isFull())
	return false;

    /*
     * Data coding
     */

    if (data & 0x0101) {
	/*
	 * This value has the reserved LSBs set, so it can't be
	 * encoded as a 14-bit index.  The only way to encode this
	 * value is as a full 16-bit literal.
	 */

	// Be careful with buffer space, since this is a huge code!
	flushDSRuns();
	txBits.flush(buf);
	if (buf.isFull())
	    return false;

	txBits.append(0x23 | (data << 8), 24);
	txBits.flush(buf);

	codePtrAdd(1);
	codeS = 0;
	codeD = RF_VRAM_DIFF_BASE;

	return true;
    }

    /*
     * Yes, this is a valid 14-bit index.
     *
     * See if we can encode it as a delta or copy from one of our four
     * sample points.  If we find a copy, that always wins and we can
     * exit early. Otherwise, see if any of the deltas are small
     * enough to encode.
     */

    uint16_t index = wordToIndex(data);

    unsigned s0 = deltaSample(vb, index, RF_VRAM_SAMPLE_0);
    if (s0 == RF_VRAM_DIFF_BASE) {
	encodeDS(s0, 0);
	txBits.flush(buf); 
	return true;
    }
    
    unsigned s1 = deltaSample(vb, index, RF_VRAM_SAMPLE_1);
    if (s1 == RF_VRAM_DIFF_BASE) {
	encodeDS(s1, 1);
	txBits.flush(buf); 
	return true;
    }

    unsigned s2 = deltaSample(vb, index, RF_VRAM_SAMPLE_2);
    if (s2 == RF_VRAM_DIFF_BASE) {
	encodeDS(s2, 2);
	txBits.flush(buf); 
	return true;
    }

    unsigned s3 = deltaSample(vb, index, RF_VRAM_SAMPLE_3);
    if (s3 == RF_VRAM_DIFF_BASE) {
	encodeDS(s3, 3);
	txBits.flush(buf); 
	return true;
    }

    if (s0 < 0x10) {
	encodeDS(s0, 0);
	txBits.flush(buf); 
	return true;
    }
    if (s1 < 0x10) {
	encodeDS(s1, 1);
	txBits.flush(buf); 
	return true;
    }
    if (s2 < 0x10) {
	encodeDS(s2, 2);
	txBits.flush(buf); 
	return true;
    }
    if (s3 < 0x10) {
	encodeDS(s3, 3);
	txBits.flush(buf); 
	return true;
    }

    /*
     * No delta found. Encode as a 14-bit literal.
     */

    // Be careful with buffer space, since this is a huge code!
    flushDSRuns();
    txBits.flush(buf);
    if (buf.isFull())
	return false;

    txBits.append(0xc | (index >> 12) | ((index & 0xFFF) << 4), 16);
    txBits.flush(buf);

    codePtrAdd(1);
    codeS = 0;
    codeD = RF_VRAM_DIFF_BASE;

    return true;
}

void CubeCodec::encodeDS(uint8_t d, uint8_t s)
{
    if (d == codeD && s == codeS && codeRuns != RF_VRAM_MAX_RUN) {
	// Extend an existing run
	codeRuns++;

    } else {
	// Can't combine with our previous run
	flushDSRuns();

	if (d == RF_VRAM_DIFF_BASE) {
	    // Copy code
	    txBits.append(0x4 | s, 4);
	} else {
	    // Diff code
	    txBits.append(0x8 | s | (d << 4), 8);
	}
	
	codeD = d;
	codeS = s;
    }

    codePtrAdd(1);
}

void CubeCodec::flushDSRuns()
{
    /*
     * Emit a run code. This should only be called if we know that we
     * can buffer another code without ending the packet short- since
     * a run isn't actually processed until a subsequent non-run code
     * occurs, due to the way we use doubled runs as special tokens.
     */

    if (codeRuns) {
	uint8_t r = codeRuns - 1;
	codeRuns = 0;

	if (r < 4) {
	    // Short run
	    txBits.append(r, 4);
	} else {
	    // Longer run
	    r -= 4;
	    txBits.append(0x2 | ((r << 8) & 0xF00) | (r & 0x30), 12);
	}

	// It isn't allowed to have two consecutive run tokens. Make sure that doesn't happen
	codeS = -1;
    }
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
