/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _VRAM_H
#define _VRAM_H

#include <sifteo/abi.h>
#include <sifteo/machine.h>

/**
 * Utilities for accessing VRAM buffers.
 *
 * These are all operations that *could* be implemented within the
 * runtime, as usermode does have access to the VRAM buffer memory.
 * But we'd usually be doing this from inside the firmware, for
 * performance reasons. There are also a few places where we do want
 * to access VRAM directly from the firmware.
 *
 * When possible, these internal utilities mirror the ones available
 * to userspace in the VideoBuffer class. But these operate at a
 * slightly lower level. For more documentation on the semantics of
 * these operations, see the userlevel VideoBuffer class.
 */

struct VRAM {

    static uint32_t &selectCM1(_SYSVideoBuffer &vbuf, uint16_t addr) {
        return vbuf.cm1[addr >> 5];
    }

    static uint32_t maskCM1(uint16_t addr) {
        return Sifteo::Intrinsic::LZ(addr & 31);
    }

    static uint32_t maskCM32(uint16_t addr) {
        return Sifteo::Intrinsic::LZ(addr >> 5);
    }

    static uint32_t maskLock(uint16_t addr) {
        return Sifteo::Intrinsic::LZ(addr >> 4);
    }

    static void truncateByteAddr(uint16_t &addr) {
        addr &= _SYS_VRAM_SIZE - 1;
    }

    static void truncateWordAddr(uint16_t &addr) {
        addr &= _SYS_VRAM_SIZE/2 - 1;
    }

    static void lock(_SYSVideoBuffer &vbuf, uint16_t addr) {
        vbuf.lock |= maskLock(addr);
        vbuf.cm32next |= maskCM32(addr);
        Sifteo::Atomic::Barrier();
    }

    static void unlock(_SYSVideoBuffer &vbuf) {
        Sifteo::Atomic::Barrier();
        Sifteo::Atomic::Or(vbuf.cm32, vbuf.cm32next);
        vbuf.lock = 0;
        Sifteo::Atomic::Or(vbuf.needPaint, vbuf.cm32next);
        vbuf.cm32next = 0;
    }

    static void poke(_SYSVideoBuffer &vbuf, uint16_t addr, uint16_t word) {
        if (vbuf.vram.words[addr] != word) {
            lock(vbuf, addr);
            vbuf.vram.words[addr] = word;
            Sifteo::Atomic::Or(selectCM1(vbuf, addr), maskCM1(addr));
        }
    }

    static void pokeb(_SYSVideoBuffer &vbuf, uint16_t addr, uint8_t byte) {
        if (vbuf.vram.bytes[addr] != byte) {
            uint16_t addrw = addr >> 1;
            lock(vbuf, addrw);
            vbuf.vram.bytes[addr] = byte;
            Sifteo::Atomic::Or(selectCM1(vbuf, addrw), maskCM1(addrw));
        }
    }

    static uint16_t peek(const _SYSVideoBuffer &vbuf, uint16_t addr) {
        return vbuf.vram.words[addr];
    }

    static uint8_t peekb(const _SYSVideoBuffer &vbuf, uint16_t addr) {
        return vbuf.vram.bytes[addr];
    }

    static void xorb(_SYSVideoBuffer &vbuf, uint16_t addr, uint8_t byte) {
        pokeb(vbuf, addr, peekb(vbuf, addr) ^ byte);
    }

    static void init(_SYSVideoBuffer &vbuf) {
        vbuf.lock = 0xFFFFFFFF;
        vbuf.cm32next = 0xFFFFFFFF;
        for (unsigned i = 0; i < arraysize(vbuf.cm1); i++)
            vbuf.cm1[i] = 0xFFFFFFFF;
    }

};

#endif
