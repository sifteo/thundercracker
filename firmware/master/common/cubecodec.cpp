/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <protocol.h>
#include "machine.h"
#include "cubecodec.h"
#include "cubeslots.h"
#include "svmmemory.h"
#include "tasks.h"

using namespace Intrinsic;

uint16_t CubeCodec::exemptionBegin;
uint16_t CubeCodec::exemptionEnd;


bool CubeCodec::encodeVRAM(PacketBuffer &buf, _SYSVideoBuffer *vb)
{
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
     *
     * Returns true iff all VRAM has been flushed. The caller should try
     * to send this additional data, if there's still room in the TX
     * buffer.
     */

    bool flushed = false;

    // Emit buffered bits from the previous packet
    txBits.flush(buf);

    if (vb) {
        /*
         * Clear the lock exemption range. This tracks words that, despite
         * being locked or dirty, we can rely on during encode because we
         * encoded them already during the same packet. This could be an
         * arbitrary set of addresses, but to keep tracking simple we only
         * track the last contiguous range of addresses.
         *
         * This helps us create good delta codes for runs that occur in
         * data which is changing too fast to fully synchronize between frames.
        */
        exemptionBegin = exemptionEnd = (uint16_t) -1;

        do {
            uint32_t cm16 = vb->cm16;
            if (!cm16)
                break;

            uint32_t idx32 = CLZ(cm16) >> 1;
            ASSERT(idx32 < arraysize(vb->cm1));
            uint32_t cm1 = vb->cm1[idx32];

            DEBUG_LOG(("CODEC[%p] cm16=%08x cm1[%d]=%08x\n", vb, cm16, idx32, cm1));

            if (cm1) {
                uint32_t idx1 = CLZ(cm1);
                uint16_t addr = (idx32 << 5) | idx1;

                ASSERT(addr < _SYS_VRAM_WORDS);
                CODEC_DEBUG_LOG(("CODEC: -encode addr %04x, data %04x\n", addr, vb->vram.words[addr]));

                if (!encodeVRAMAddr(buf, addr) ||
                    !encodeVRAMData(buf, vb, VRAM::peek(*vb, addr))) {

                    /*
                     * We ran out of room to encode. This should be rare,
                     * happening only when we're near the end of the
                     * packet buffer AND we're encoding a very large code,
                     * like a literal 16-bit write plus a literal address
                     * change.
                     */
                    break;
                }

                // Extend or reset the exemption range.
                if (addr != exemptionEnd)
                    exemptionBegin = addr;
                exemptionEnd = addr + 1;

                cm1 &= ROR(0x7FFFFFFF, idx1);
                vb->cm1[idx32] = cm1;
            }

            if (!cm1) {
                // We operate at a 1:32 resolution, half that of the cm16.
                // So, clear two bits at a time.
                cm16 &= ROR(0x3FFFFFFF, idx32 << 1);
                vb->cm16 = cm16;
                DEBUG_LOG(("CODEC[%p] cm16=%08x, cm1 cleared\n", vb, cm16));
                if (!cm16)
                    flushed = true;
            }
        } while (!buf.isFull());
    }

    /*
     * If we have room in the buffer, and nothing left to render,
     * see if we can flush leftover bits out to the hardware. We may
     * have residual bits in txBits, and we may have to emit a run.
     */

    // Emit buffered bits
    txBits.flush(buf);

    // Flush out an RLE run, if one is buffered
    if (!buf.isFull()) {
        flushDSRuns(true);
        txBits.flush(buf);
    }

    return flushed;
}

bool CubeCodec::encodeVRAMAddr(PacketBuffer &buf, uint16_t addr)
{
    ASSERT(addr < _SYS_VRAM_WORDS);
    ASSERT(codePtr < _SYS_VRAM_WORDS);

    if (addr != codePtr) {
        uint16_t delta = (addr - codePtr) & _SYS_VRAM_WORD_MASK;

        flushDSRuns(true);
        txBits.flush(buf);
        if (buf.isFull())
            return false;

        if (delta <= 8) {
            // We can use a short skip code

            CODEC_DEBUG_LOG(("CODEC: addr delta %d\n", delta));

            delta--;
            txBits.append((delta & 1) | ((delta << 3) & 0x30), 8);
            txBits.flush(buf);

        } else {
            // Too large a delta, use a longer literal code

            CODEC_DEBUG_LOG(("CODEC: addr literal %04x\n", addr));

            txBits.append(3 | ((addr >> 4) & 0x10) | (addr & 0xFF) << 8, 16);
            txBits.flush(buf);
        }

        codePtr = addr;
    }

    return true;
}

bool CubeCodec::encodeVRAMData(PacketBuffer &buf, _SYSVideoBuffer *vb, uint16_t data)
{
    /*
     * This version of encodeVRAMData() uses a VideoBuffer to do
     * delta encoding if possible. Without a VideoBuffer, the data
     * will be sent without the use of compressed delta codes
     */

    if (buf.isFull())
        return false;

    /*
     * See if we can encode this word as a delta or copy from one of
     * our four sample points.  If we find a copy, that always wins
     * and we can exit early. Otherwise, see if any of the deltas are
     * small enough to encode.
     */

    unsigned s0 = deltaSample(vb, data, RF_VRAM_SAMPLE_0);
    if (s0 == RF_VRAM_DIFF_BASE) {
        encodeDS(s0, 0);
        txBits.flush(buf); 
        return true;
    }
    
    unsigned s1 = deltaSample(vb, data, RF_VRAM_SAMPLE_1);
    if (s1 == RF_VRAM_DIFF_BASE) {
        encodeDS(s1, 1);
        txBits.flush(buf); 
        return true;
    }

    unsigned s2 = deltaSample(vb, data, RF_VRAM_SAMPLE_2);
    if (s2 == RF_VRAM_DIFF_BASE) {
        encodeDS(s2, 2);
        txBits.flush(buf); 
        return true;
    }

    unsigned s3 = deltaSample(vb, data, RF_VRAM_SAMPLE_3);
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

    // No delta found. Try a literal encoding
    return encodeVRAMData(buf, data);
}

bool CubeCodec::encodeVRAMData(PacketBuffer &buf, uint16_t data)
{
    /*
     * This version does not require a VideoBuffer, but the data will be sent
     * without using any delta encoding.
     */

    if (buf.isFull())
        return false;

    if (data & 0x0101) {
        // 16-bit literal

        flushDSRuns(true);
        txBits.flush(buf);
        if (buf.isFull())
            return false;

        CODEC_DEBUG_LOG(("CODEC: data literal-16 %04x\n", data));

        txBits.append(0x23 | (data << 8), 24);
        txBits.flush(buf);

        codePtrAdd(1);
        codeS = 0;
        codeD = RF_VRAM_DIFF_BASE;

    } else {
        // 14-bit literal

        /*
         * We're guaranteeing that this flushDSRuns will be
         * immediately followed by the literal code. We can't return
         * false between the two, since that gives up our ability to
         * keep this sequence of codes atomic.
         */
        flushDSRuns(false);

        uint16_t index = ((data & 0xFF) >> 1) | ((data & 0xFF00) >> 2);
        CODEC_DEBUG_LOG(("CODEC: data literal-14 %04x\n", index));

        txBits.append(0xc | (index >> 12) | ((index & 0xFFF) << 4), 16);
        txBits.flush(buf);

        codePtrAdd(1);
        codeS = 0;
        codeD = RF_VRAM_DIFF_BASE;
    }

    return true;
}

void CubeCodec::encodeDS(uint8_t d, uint8_t s)
{
    ASSERT(codeRuns <= RF_VRAM_MAX_RUN);

    if (d == codeD && s == codeS && codeRuns != RF_VRAM_MAX_RUN) {
        // Extend an existing run
        codeRuns++;

    } else {
        flushDSRuns(false);

        CODEC_DEBUG_LOG(("CODEC: ds %d %d\n", d, s));
        appendDS(d, s);
        codeD = d;
        codeS = s;
    }

    codePtrAdd(1);
}

void CubeCodec::flushDSRuns(bool rleSafe)
{
    /*
     * Emit a run code.
     *
     * Because we treat doubled run codes as a short form of escape, we
     * have to be very careful about which codes we emit immediately after
     * a flushDSRuns(). It is ONLY safe to emit an RLE code if the next
     * code we emit is a copy, a diff, or a literal index code.
     *
     * If 'rleSafe' is false, the caller is guaranteeing that the next
     * code is already safe. If not, we will perform a simple transformation
     * on the run code. The last run will be converted back to a non-RLE
     * copy or diff code.
     *
     * It is NOT acceptable to call flushDSRuns with rleSafe==FALSE then
     * to return due to an end-of-packet. We can't guarantee that the next
     * packet will resume with the same kind of code, so the caller isn't
     * upholding its side of this contract.
     */

    ASSERT(codeRuns <= RF_VRAM_MAX_RUN);

    if (codeRuns) {
        CODEC_DEBUG_LOG(("CODEC: flush-ds d=%d s=%d x%d, rs=%d\n", codeD, codeS, codeRuns, rleSafe));

        // Save room for the trailing non-RLE code
        if (rleSafe)
            codeRuns--;

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
        }

        // Trailing non-RLE code
        if (rleSafe)
            appendDS(codeD, codeS);

        // Can't emit another run immediately after a flush
        codeD = -1;
    }
}

void CubeCodec::endPacket(PacketBuffer &buf)
{
    /*
     * Big note:
     *   - If we didn't emit a full packet, that implies an encoder state reset.
     *   - Period. It doesn't matter what's in the packet, just whether or not
     *     it's full!
     *
     * End conditions:
     *   - Full packet. We already totally filled the buffer, and we may have
     *     additional bits saved in txBits for later. We're done.
     *   - Single nybble buffered on a not-yet-full packet. We can flush that
     *     nybble by adding a 'junk' nybble afterwards: anything that doesn't
     *     represent a full code.
     *   - BUT, if we happen to end up with a full packet after padding, there
     *     is no state reset afterwards and this junk nybble will be interpreted
     *     as part of the next packet.
     *
     * SO, we do need to pad, but we may not be able to pad with pure junk.
     * If we're in this last situation, we can pad with some arbitrary
     * harmless code, like a skip (00), as long as we keep the encode and
     * decode state in sync.
     */

    if (!buf.isFull()) {
        // If not full, pad with a single nybble 0. This is usable as a junk
        // nybble, plus it can be the beginning of a skip code if necessary.

        ASSERT(codeRuns == 0);

        if (txBits.hasPartialByte()) {
            txBits.append(0, 4);
            txBits.flush(buf);
        }

        if (buf.isFull()) {
            // That just filled up the packet! No state reset, and gear up
            // to begin the next packet with a tiny skip code (00).
            
            codePtr = (codePtr + 1) & _SYS_VRAM_WORD_MASK;
            txBits.append(0, 4);
            CODEC_DEBUG_LOG(("CODEC: Padded with split short skip code\n"));

        } else {
            // Still not full. This will be a state reset, and we can discard
            // the nybble above as junk.

            if (!buf.len) {
                /*
                 * If we have nothing to send, make it an empty 'ping' packet.
                 * But the nRF24L01 can't actually send a zero-byte packet, so
                 * we have to pad it with a no-op. We don't have any explicit
                 * no-op in our protocol, but we can send only the first byte
                 * from a multi-byte code.
                 *
                 * This is the first byte of a 14-bit literal.
                 *
                 * Note that we may have buffered some bits ahead of this,
                 * such as the second nybble of the split skip code above.
                 * This should be fine- worst case, we will end up discarding
                 * the second nybble of this ping code.
                 *
                 * stateReset() must come after this, not before. It discards
                 * buffered bits, and we must not do that prior to emitting
                 * the ping.
                 */
                
                txBits.append(0xFF, 8);
                txBits.flush(buf); 
            }

            stateReset();
        }
    }
}

unsigned CubeCodec::deltaSample(_SYSVideoBuffer *vb, uint16_t data, uint16_t offset)
{
    uint16_t ptr = codePtr - offset;
    ptr &= _SYS_VRAM_WORD_MASK;

    CODEC_DEBUG_LOG(("CODEC: deltaSample(%04x, %03x) "
        "lock=%08x mask=%08x codePtr=%03x\n",
        data, offset, vb->lock, VRAM::maskCM16(ptr), codePtr));

    if ((vb->lock & VRAM::maskCM16(ptr)) ||
        (VRAM::selectCM1(*vb, ptr) & VRAM::maskCM1(ptr))) {

        // This word is locked, but it may be exempt from the
        // lock because we've encoded it during this very same packet
        // (while we have userspace blocked, and the vbuf can't change).
        
        if (ptr < exemptionBegin || ptr >= exemptionEnd) {
            // Not exempt. Can't match a locked or modified word
            return (unsigned) -1;
        }
    }

    uint16_t sample = VRAM::peek(*vb, ptr);

    if ((sample & 0x0101) != (data & 0x0101)) {
        // Different LSBs, can't possibly reach it via a delta
        return (unsigned) -1;
    }

    int16_t dI = _SYS_INVERSE_TILE77(data);
    int16_t sI = _SYS_INVERSE_TILE77(sample);

    // Note that our delta codes leave the LSBs untouched
    ASSERT(_SYS_TILE77(dI) == (data & 0xFEFE));
    ASSERT(_SYS_TILE77(sI) == (sample & 0xFEFE));

    unsigned result = dI - sI + RF_VRAM_DIFF_BASE;

    CODEC_DEBUG_LOG(("CODEC: deltaSample(%04x, %03x) "
        "sample=%04x dI=%04x sI=%04x res=%d\n",
        data, offset, sample, dI, sI, result));

    return result;
}

void CubeCodec::escTimeSync(PacketBuffer &buf, uint16_t rawTimer)
{
    /*
     * Timer synchronization escape. This sends the sync escape,
     * plus a dummy nybble to force a flush if necessary. We then
     * send the new raw 13-bit time synchronization value. This
     * must be the last code in the packet.
     */

    ASSERT(txBits.hasRoomForFlush(buf, 12 + 2*8));

    txBits.append(0xF78, 12);
    txBits.flush(buf);
    txBits.init();

    buf.append(rawTimer & 0x1F);    // Low 5 bits
    buf.append(rawTimer >> 5);      // High 8 bits

    if (!buf.isFull())
        stateReset();
}

bool CubeCodec::escRequestAck(PacketBuffer &buf)
{
    /*
     * If the buffer has room, adds an "Explicit ACK request" escape and
     * returns true. Otherwise, returns false.
     */
    if (txBits.hasRoomForFlush(buf, 12)) {

        txBits.append(0xF79, 12);
        txBits.flush(buf);
        txBits.init();

        if (!buf.isFull())
            stateReset();

        return true;
    }
    return false;
}

bool CubeCodec::escFlash(PacketBuffer &buf)
{
    /*
     * If the buffer has room for the escape and at least one data byte,
     * adds a "Flash Escape" command to the buffer, (two-nybble code 33)
     * plus one extra dummy nybble to force a byte flush if necessary.
     *
     * This implies an encoder state reset.
     */

    if (txBits.hasRoomForFlush(buf, 12 + 8)) {
    
        txBits.append(0xF33, 12);
        txBits.flush(buf);
        txBits.init();

        ASSERT(!buf.isFull());
        stateReset();

        return true;
    }
    return false;
}

bool CubeCodec::escRadioNap(PacketBuffer &buf, uint16_t duration)
{
    /*
     * If the buffer has room, add a "Radio Nap" code, instructing
     * the cube to turn its receiver off for 'duration' CLKLF ticks.
     */

    if (txBits.hasRoomForFlush(buf, 12 + 2*8)) {

        txBits.append(0xF7B, 12);
        txBits.flush(buf);
        txBits.init();

        buf.append(duration);
        buf.append(duration >> 8);

        if (!buf.isFull())
            stateReset();

        return true;
    }
    return false;
}

void CubeCodec::encodeShutdown(PacketBuffer &buf)
{
    /*
     * Tell this cube to power off, by sending it to _SYS_VM_SLEEP.
     * We leave this bit set; if the cube doesn't go to sleep right
     * away, keep telling it to. Once we succeed, the cube will
     * disconnect.
     */

    encodePoke(buf, offsetof(_SYSVideoRAM, mode)/2,
        _SYS_VM_SLEEP | (_SYS_VF_CONTINUOUS << 8));
}

void CubeCodec::encodeStipple(PacketBuffer &buf, _SYSVideoBuffer *vbuf)
{
    /* 
     * Show that this cube is paused, by having it draw a stipple pattern.
     *
     * Note, we expect this cube to have finished rendering already,
     * or it may end up drawing a garbage frame behind the stipple!
     *
     * This does not require us to have a vbuf attached. But if we
     * do, we'll use it to be a good citizen and avoid using continuous
     * mode or causing an unintentional rotation change.
     */

    unsigned modeFlagsWord = _SYS_VM_STAMP;
    if (vbuf) {
        uint8_t flags = vbuf->vram.flags ^ _SYS_VF_TOGGLE;
        vbuf->flags = flags;
        modeFlagsWord |= flags << 8;
    } else {
        modeFlagsWord |= _SYS_VF_CONTINUOUS << 8;
    }

    encodePoke(buf, offsetof(_SYSVideoRAM, fb)/2,          0x0220);
    encodePoke(buf, offsetof(_SYSVideoRAM, colormap[2])/2, 0x0000);
    encodePoke(buf, offsetof(_SYSVideoRAM, stamp_pitch)/2, 0x0201);
    encodePoke(buf, offsetof(_SYSVideoRAM, stamp_x)/2,     0x8000);
    encodePoke(buf, offsetof(_SYSVideoRAM, stamp_key)/2,   0x0000);
    encodePoke(buf, offsetof(_SYSVideoRAM, first_line)/2,  0x8000);
    encodePoke(buf, offsetof(_SYSVideoRAM, mode)/2,        modeFlagsWord);
}
