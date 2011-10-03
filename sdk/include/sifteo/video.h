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
        _SYS_vbuf_lock(&sys, addr);
    }

    /**
     * End a sequence of modifications to VRAM. If the system is not
     * already busy flushing updates to the cube, this allows it to begin.
     */
    void unlock() {
        _SYS_vbuf_unlock(&sys);
    }

    /**
     * Mark the VideoBuffer as having changed, without actually modifying
     * any memory. This will force the next System::paint() to actually
     * redraw this cube, even if it seems like nothing has changed.
     */
    void touch() {
        Sifteo::Atomic::Or(sys.needPaint, 1);
    }

    /**
     * Modify a word of VRAM, automatically locking it and marking the
     * change only if that word has actually been modified. After a
     * sequence of poke() calls, the caller is responsible for issuing
     * one unlock().
     */
    void poke(uint16_t addr, uint16_t word) {
        _SYS_vbuf_poke(&sys, addr, word);
    }

    /**
     * Like poke(), but modifies a single byte. Less efficient, but
     * sometimes you really do just want to modify one byte.
     */
    void pokeb(uint16_t addr, uint8_t byte) {
        _SYS_vbuf_pokeb(&sys, addr, byte);
    }

    /**
     * Poke a 14-bit tile index into a particular VRAM word.
     */
    void pokei(uint16_t addr, uint16_t index) {
        _SYS_vbuf_poke(&sys, addr, ((index << 2) & 0xFE00) | ((index << 1) & 0x00FE));
    }

    /**
     * Read one word of VRAM
     */
    uint16_t peek(uint16_t addr) const {
        uint16_t word;
        _SYS_vbuf_peek(&sys, addr, &word);
        return word;
    }

    /**
     * Read one byte of VRAM
     */
    uint8_t peekb(uint16_t addr) const {
        uint8_t byte;
        _SYS_vbuf_peekb(&sys, addr, &byte);
        return byte;
    }

    /**
     * Initialize the buffer. Note that this doesn't initialize the *contents*
     * of VRAM, as that's format specific. This just initializes the change maps,
     * so that on the next unlock() we'll send the entire buffer.
     */
    void init() {
        _SYS_vbuf_init(&sys);
    }

    _SYSVideoBuffer sys;    
};


};  // namespace Sifteo

#endif
