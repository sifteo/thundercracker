/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_VIDEO_H
#define _SIFTEO_VIDEO_H

#include <sifteo/macros.h>
#include <sifteo/machine.h>

namespace Sifteo {


/**
 * A memory buffer which holds graphics data. This is a mirror of the
 * remote graphics memory in each cube's hardware. By writing to this
 * buffer, changes are enqueued for later transmission to the physical
 * video buffer.
 *
 * This class only handles low-level memory management. The layout of
 * this memory is specific to the video mode you're using.
 *
 * If you're accessing the VideoBuffer directly, it is strongly
 * recommended that you're aware of the synchronization protocol used
 * in this buffer, to keep it consistent between the system software
 * and your application.  See abi.h for details on this protocol.
 */

class VideoBuffer {
 public:

    /**
     * Prepare to modify a particular address. Addresses must be
     * locked prior to the buffer memory being modified. Multiple
     * calls to lock may be made in a row, without an intervening
     * unlock().
     */
    void lock(uint16_t addr) {
        sys.lock |= maskLock(addr);
        nextCM32 |= maskCM32(addr);
        Atomic::Barrier();
    }

    /**
     * End a sequence of modifications to VRAM. If the system is not
     * already busy flushing updates to the cube, this allows it to begin.
     */
    void unlock() {
        Atomic::Barrier();
        Atomic::Or(sys.cm32, nextCM32);
        sys.lock = 0;
        nextCM32 = 0;
    }

    /**
     * Modify a word of VRAM, automatically locking it and marking the
     * change only if that word has actually been modified. After a
     * sequence of poke() calls, the caller is responsible for issuing
     * one unlock().
     */
    void poke(uint16_t addr, uint16_t word) {
        if (sys.vram.words[addr] != word) {
            lock(addr);
            sys.vram.words[addr] = word;
            Atomic::Or(selectCM1(addr), maskCM1(addr));
        }
    }

    /**
     * Like poke(), but modifies a single byte. Less efficient, but
     * sometimes you really do just want to modify one byte.
     */
    void pokeb(uint16_t addr, uint8_t byte) {
        if (sys.vram.bytes[addr] != byte) {
            uint16_t addrw = addr >> 1;
            lock(addrw);
            sys.vram.bytes[addr] = byte;
            Atomic::Or(selectCM1(addrw), maskCM1(addrw));
        }
    }

    /**
     * Read one word of VRAM
     */
    uint16_t peek(uint16_t addr) const {
        return sys.vram.words[addr];
    }

    /**
     * Read one byte of VRAM
     */
    uint8_t peekb(uint16_t addr) const {
        return sys.vram.bytes[addr];
    }

    /**
     * Initialize the buffer. Note that this doesn't initialize the *contents*
     * of VRAM, as that's format specific. This just initializes the change maps,
     * so that on the next unlock() we'll send the entire buffer.
     */
    void init() {
        sys.lock = 0xFFFFFFFF;
        nextCM32 = 0xFFFFFFFF;
        for (unsigned i = 0; i < arraysize(sys.cm1); i++)
            sys.cm1[i] = 0xFFFFFFFF;
    }

    _SYSVideoBuffer sys;    
    
 private:
    uint32_t &selectCM1(uint16_t addr) {
        return sys.cm1[addr >> 5];
    }

    uint32_t maskCM1(uint16_t addr) {
        return Intrinsic::LZ(addr & 31);
    }

    uint32_t maskCM32(uint16_t addr) {
        return Intrinsic::LZ(addr >> 5);
    }

    uint32_t maskLock(uint16_t addr) {
        return Intrinsic::LZ(addr >> 4);
    }

    uint32_t nextCM32;
};


};  // namespace Sifteo

#endif
